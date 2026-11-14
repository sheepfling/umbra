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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks queued receive-order delivery and the earliest NMR TSO cohorts. */
final class NmrQueuedMessageSetTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long PENDING_WINDOW_NANOS = 40_000_000L;

   private NmrQueuedMessageSetTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder publisherCallbacks = new Recorder();
      String federation = "java-tck-nmr-queued-message-set-" + UUID.randomUUID();
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
         requester.joinFederationExecution("java-tck-nmr-queued-requester", federation);
         requesterJoined = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-nmr-queued-publisher", federation);
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
            "NMR queued-message FOM did not resolve matching interaction handles");
         requester.subscribeInteractionClass(requesterInteraction);
         requesterSubscribed = true;
         publisher.publishInteractionClass(publisherInteraction);
         publisherPublished = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         JavaTckSupport.check(requesterFactory.getName().equals(publisherFactory.getName()),
            "NMR queued-message members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(publisherInitial),
            "NMR queued-message members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();

         requester.enableTimeConstrained();
         requesterConstrained = true;
         await(requester, () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "NMR queued-message requester Time Constrained callback");
         publisher.enableTimeRegulation(publisherEpsilon);
         publisherRegulating = true;
         await(publisher, () -> publisherCallbacks.timeRegulationEnabled == 1,
            "NMR queued-message publisher Time Regulation callback");

         LogicalTime<?, ?> timeFive = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime<?, ?> timeEight = offset(publisherInitial, publisherEpsilon, 8);
         LogicalTime<?, ?> timeTwelve = offset(publisherInitial, publisherEpsilon, 12);
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> publisherTargetSix =
            offset(publisherInitial, publisherEpsilon, 6);
         LogicalTime<?, ?> publisherTargetNine =
            offset(publisherInitial, publisherEpsilon, 9);
         LogicalTime<?, ?> publisherTargetEleven =
            offset(publisherInitial, publisherEpsilon, 11);

         byte[] payload = {0x4e, 0x4d, 0x52};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload.clone());
         publisher.sendInteraction(publisherInteraction, values, new byte[] {0x11});
         publisher.sendInteraction(publisherInteraction, values, new byte[] {0x12});

         Map<Integer, MessageRetractionHandle> expectedRetractions = new HashMap<>();
         expectedRetractions.put(0x21, sendTimestamped(
            publisher, publisherInteraction, values, new byte[] {0x21}, timeFive));
         expectedRetractions.put(0x22, sendTimestamped(
            publisher, publisherInteraction, values, new byte[] {0x22}, timeFive));
         expectedRetractions.put(0x28, sendTimestamped(
            publisher, publisherInteraction, values, new byte[] {0x28}, timeEight));
         expectedRetractions.put(0x2c, sendTimestamped(
            publisher, publisherInteraction, values, new byte[] {0x2c}, timeTwelve));

         JavaTckSupport.check(requesterCallbacks.receiveOrder.isEmpty()
               && requesterCallbacks.timestamped.isEmpty(),
            "NMR queued-message requester observed input before invoking NMR");
         requester.nextMessageRequest(requestTime);
         await(requester, () -> requesterCallbacks.receiveOrder.size() >= 2,
            "NMR queued receive-order interactions");
         requirePending(requester, publisher, requesterCallbacks,
            2, 0, 0, "NMR completed before its first queued TSO became safe");
         JavaTckSupport.check(requesterCallbacks.receiveOrder.size() == 2
               && requesterCallbacks.timestamped.isEmpty()
               && requesterCallbacks.grants.isEmpty(),
            "NMR delivered a TSO or granted before its source advanced");
         List<Integer> receiveOrderTags = tags(requesterCallbacks.receiveOrder);
         JavaTckSupport.check(receiveOrderTags.equals(Arrays.asList(0x11, 0x12)),
            "NMR did not deliver both queued receive-order interactions");
         for (InteractionRecord event : requesterCallbacks.receiveOrder) {
            checkInteraction(event, requesterInteraction, requesterParameter,
               publisherHandle, payload);
         }

         publisher.timeAdvanceRequest(publisherTargetSix);
         awaitBoth(requester, publisher,
            () -> requesterCallbacks.timestamped.size() >= 2
               && requesterCallbacks.grants.size() >= 1
               && publisherCallbacks.grants.size() >= 1,
            "NMR earliest timestamp-5 cohort and grant");
         JavaTckSupport.check(requesterCallbacks.timestamped.size() == 2
               && requesterCallbacks.grants.size() == 1
               && publisherCallbacks.grants.size() == 1,
            "NMR did not deliver exactly the earliest equal-time TSO cohort");
         JavaTckSupport.check(tags(requesterCallbacks.timestamped)
               .equals(Arrays.asList(0x21, 0x22)),
            "NMR did not deliver both timestamp-5 interactions");
         for (InteractionRecord event : requesterCallbacks.timestamped) {
            int tag = onlyTag(event, "NMR timestamp-5 cohort");
            checkTimedInteraction(event, requesterInteraction, requesterParameter,
               publisherHandle, payload, timeFive, expectedRetractions.get(tag));
         }
         JavaTckSupport.check(requesterCallbacks.callbackOrder.equals(Arrays.asList(
               "interaction", "interaction", "interaction", "interaction", "grant"))
               && timeFive.equals(requesterCallbacks.grants.get(0))
               && publisherTargetSix.equals(publisherCallbacks.grants.get(0))
               && timeFive.equals(requester.queryLogicalTime()),
            "NMR did not deliver the queued input before granting at the earliest TSO time");

         requester.nextMessageRequest(requestTime);
         requirePending(requester, publisher, requesterCallbacks,
            2, 2, 1, "NMR selected a later TSO before its source advanced");
         JavaTckSupport.check(requesterCallbacks.receiveOrder.size() == 2
               && requesterCallbacks.timestamped.size() == 2
               && requesterCallbacks.grants.size() == 1,
            "NMR selected a later TSO before its source advanced");
         publisher.timeAdvanceRequest(publisherTargetNine);
         awaitBoth(requester, publisher,
            () -> requesterCallbacks.timestamped.size() >= 3
               && requesterCallbacks.grants.size() >= 2
               && publisherCallbacks.grants.size() >= 2,
            "NMR follow-on timestamp-8 interaction");
         JavaTckSupport.check(requesterCallbacks.timestamped.size() == 3
               && requesterCallbacks.grants.size() == 2
               && publisherCallbacks.grants.size() == 2,
            "NMR duplicated or omitted messages on its second request");
         InteractionRecord timeEightEvent = requesterCallbacks.timestamped.get(2);
         JavaTckSupport.check(onlyTag(timeEightEvent, "NMR timestamp-8 message") == 0x28,
            "NMR selected a message other than the next timestamp-8 interaction");
         checkTimedInteraction(timeEightEvent, requesterInteraction, requesterParameter,
            publisherHandle, payload, timeEight, expectedRetractions.get(0x28));
         JavaTckSupport.check(timeEight.equals(requesterCallbacks.grants.get(1))
               && publisherTargetNine.equals(publisherCallbacks.grants.get(1))
               && timeEight.equals(requester.queryLogicalTime())
               && requesterCallbacks.callbackOrder.equals(Arrays.asList(
                  "interaction", "interaction", "interaction", "interaction", "grant",
                  "interaction", "grant")),
            "NMR did not retire the first cohort before granting at timestamp 8");

         requester.nextMessageRequest(requestTime);
         requirePending(requester, publisher, requesterCallbacks,
            2, 3, 2, "NMR selected a timestamp beyond the request boundary");
         JavaTckSupport.check(requesterCallbacks.timestamped.size() == 3
               && requesterCallbacks.grants.size() == 2,
            "NMR selected timestamp 12 beyond its request boundary");
         publisher.timeAdvanceRequest(publisherTargetEleven);
         awaitBoth(requester, publisher,
            () -> requesterCallbacks.grants.size() >= 3
               && publisherCallbacks.grants.size() >= 3,
            "NMR request-boundary grant with timestamp 12 still queued");
         JavaTckSupport.check(requesterCallbacks.timestamped.size() == 3
               && requesterCallbacks.grants.size() == 3
               && publisherCallbacks.grants.size() == 3
               && requestTime.equals(requesterCallbacks.grants.get(2))
               && publisherTargetEleven.equals(publisherCallbacks.grants.get(2))
               && requestTime.equals(requester.queryLogicalTime())
               && requesterCallbacks.callbackOrder.equals(Arrays.asList(
                  "interaction", "interaction", "interaction", "interaction", "grant",
                  "interaction", "grant", "grant")),
            "NMR did not grant at time 10 while the timestamp-12 interaction remained outside the request");

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
         InteractionClassHandle interaction, ParameterHandleValueMap values,
         byte[] tag, LogicalTime<?, ?> time) throws Exception {
      MessageRetractionReturn result = publisher.sendInteraction(
         interaction, values, tag.clone(), time);
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null,
         "timestamped NMR message-set send returned no valid retraction handle");
      return result.handle;
   }

   private static void checkInteraction(InteractionRecord event,
         InteractionClassHandle interaction, ParameterHandle parameter,
         FederateHandle publisher, byte[] payload) {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.values.size() == 1
            && Arrays.equals(event.values.get(parameter), payload)
            && publisher.equals(event.producer),
         "NMR message-set callback changed the interaction, parameter, payload, or producer");
   }

   private static void checkTimedInteraction(InteractionRecord event,
         InteractionClassHandle interaction, ParameterHandle parameter,
         FederateHandle publisher, byte[] payload, LogicalTime<?, ?> expectedTime,
         MessageRetractionHandle expectedRetraction) {
      checkInteraction(event, interaction, parameter, publisher, payload);
      JavaTckSupport.check(expectedTime.equals(event.time)
            && event.sentOrder == OrderType.TIMESTAMP
            && event.receivedOrder == OrderType.TIMESTAMP
            && expectedRetraction != null && expectedRetraction.equals(event.retraction),
         "NMR TSO callback changed its timestamp, order, or retraction metadata");
   }

   private static List<Integer> tags(List<InteractionRecord> interactions) {
      List<Integer> result = new ArrayList<>();
      for (InteractionRecord event : interactions) result.add(onlyTag(event, "NMR interaction"));
      Collections.sort(result);
      return result;
   }

   private static int onlyTag(InteractionRecord event, String description) {
      JavaTckSupport.check(event.tag.length == 1,
         description + " has a malformed interaction tag");
      return event.tag[0] & 0xff;
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

   private static void requirePending(RTIambassador requester, RTIambassador publisher,
         Recorder callbacks, int expectedReceiveOrder, int expectedTimestamped,
         int expectedGrants, String description) throws Exception {
      long deadline = System.nanoTime() + PENDING_WINDOW_NANOS;
      while (System.nanoTime() < deadline) {
         requester.evokeCallback(0.0);
         publisher.evokeCallback(0.0);
         JavaTckSupport.check(callbacks.receiveOrder.size() == expectedReceiveOrder
               && callbacks.timestamped.size() == expectedTimestamped
               && callbacks.grants.size() == expectedGrants, description);
         Thread.sleep(1L);
      }
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

   private static final class InteractionRecord {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap values;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private InteractionRecord(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         values = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         if (arguments.length == 10) {
            time = (LogicalTime<?, ?>) arguments[6];
            sentOrder = (OrderType) arguments[7];
            receivedOrder = (OrderType) arguments[8];
            retraction = (MessageRetractionHandle) arguments[9];
         } else {
            time = null;
            sentOrder = null;
            receivedOrder = (OrderType) arguments[5];
            retraction = null;
         }
      }
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private final List<InteractionRecord> receiveOrder = new CopyOnWriteArrayList<>();
      private final List<InteractionRecord> timestamped = new CopyOnWriteArrayList<>();
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
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  receiveOrder.add(new InteractionRecord(arguments));
                  callbackOrder.add("interaction");
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timestamped.add(new InteractionRecord(arguments));
                  callbackOrder.add("interaction");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK NMR message-set callbacks";
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
