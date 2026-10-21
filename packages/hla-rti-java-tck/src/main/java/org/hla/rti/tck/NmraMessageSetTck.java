package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies NMRA's queued receive-order set and successive equal-time TSO cohorts. */
final class NmraMessageSetTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NmraMessageSetTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "NMRA message-set request-boundary assertions require HLA_EVOKED");
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder publisherCallbacks = new Recorder();
      String federation = "java-tck-nmra-message-set-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean publisherConnected = false;
      boolean federationCreated = false;
      boolean requesterJoined = false;
      boolean publisherJoined = false;
      try {
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         requester.joinFederationExecution("java-tck-nmra-requester", federation);
         requesterJoined = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-nmra-publisher", federation);
         publisherJoined = true;

         InteractionClassHandle requesterInteraction =
            requester.getInteractionClassHandle(interactionName);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionName);
         ParameterHandle requesterParameter = requester.getParameterHandle(
            requesterInteraction, parameterName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         JavaTckSupport.check(requesterInteraction != null && publisherInteraction != null
               && requesterParameter != null && publisherParameter != null
               && requesterInteraction.equals(publisherInteraction)
               && requesterParameter.equals(publisherParameter),
            "NMRA message-set adapter FOM did not resolve stable interaction handles");
         requester.subscribeInteractionClass(requesterInteraction);
         publisher.publishInteractionClass(publisherInteraction);
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         JavaTckSupport.check(requesterFactory.getName().equals(publisherFactory.getName()),
            "NMRA message-set members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(publisherInitial),
            "NMRA message-set members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(requester, () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "NMRA message-set requester Time Constrained callback");
         requester.enableTimeRegulation(requesterEpsilon);
         await(requester, () -> requesterCallbacks.timeRegulationEnabled != null,
            "NMRA message-set requester Time Regulation callback");
         publisher.enableTimeRegulation(publisherEpsilon);
         await(publisher, () -> publisherCallbacks.timeRegulationEnabled != null,
            "NMRA message-set publisher Time Regulation callback");

         LogicalTime<?, ?> timeFive = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime<?, ?> timeEight = offset(publisherInitial, publisherEpsilon, 8);
         LogicalTime<?, ?> requesterTarget = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> publisherTarget = offset(publisherInitial, publisherEpsilon, 12);
         byte[] payload = {0x4e, 0x4d, 0x52, 0x41};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload.clone());
         publisher.sendInteraction(publisherInteraction, values, new byte[] {0x11});
         publisher.sendInteraction(publisherInteraction, values, new byte[] {0x12});
         publisher.sendInteraction(publisherInteraction, values, new byte[] {0x21}, timeFive);
         publisher.sendInteraction(publisherInteraction, values, new byte[] {0x22}, timeFive);
         publisher.sendInteraction(publisherInteraction, values, new byte[] {0x30}, timeEight);

         JavaTckSupport.check(requesterCallbacks.receiveOrder.isEmpty()
               && requesterCallbacks.timestamped.isEmpty(),
            "NMRA message-set requester observed messages before callback servicing");
         requester.nextMessageRequestAvailable(requesterTarget);
         await(requester, () -> requesterCallbacks.receiveOrder.size() >= 2,
            "NMRA message-set queued receive-order interactions");
         JavaTckSupport.check(requesterCallbacks.receiveOrder.size() == 2
               && requesterCallbacks.timestamped.isEmpty()
               && requesterCallbacks.grants.isEmpty(),
            "NMRA message-set delivered TSO input or granted before the publisher advanced");

         publisher.timeAdvanceRequest(publisherTarget);
         awaitBoth(requester, publisher,
            () -> requesterCallbacks.grants.size() == 1
               && publisherCallbacks.grants.size() == 1,
            "NMRA message-set first grant pair");
         JavaTckSupport.check(requesterCallbacks.receiveOrder.size() == 2
               && requesterCallbacks.timestamped.size() == 2
               && requesterCallbacks.grants.size() == 1
               && publisherCallbacks.grants.size() == 1,
            "NMRA message-set did not deliver exactly the queued receive-order set and earliest TSO cohort");
         List<Integer> receiveOrderTags = new ArrayList<>();
         for (InteractionRecord event : requesterCallbacks.receiveOrder) {
            checkInteraction(event, requesterInteraction, requesterParameter,
               publisherHandle, payload);
            JavaTckSupport.check(event.tag.length == 1,
               "NMRA message-set receive-order callback had a malformed tag");
            receiveOrderTags.add(event.tag[0] & 0xff);
         }
         Collections.sort(receiveOrderTags);
         JavaTckSupport.check(receiveOrderTags.equals(Arrays.asList(0x11, 0x12)),
            "NMRA message-set did not deliver both queued receive-order interactions");

         List<Integer> firstCohortTags = new ArrayList<>();
         for (InteractionRecord event : requesterCallbacks.timestamped) {
            checkInteraction(event, requesterInteraction, requesterParameter,
               publisherHandle, payload);
            JavaTckSupport.check(event.tag.length == 1 && timeFive.equals(event.time)
                  && event.receivedOrder == OrderType.TIMESTAMP,
               "NMRA message-set delivered a non-earliest or malformed TSO interaction");
            firstCohortTags.add(event.tag[0] & 0xff);
         }
         Collections.sort(firstCohortTags);
         JavaTckSupport.check(firstCohortTags.equals(Arrays.asList(0x21, 0x22)),
            "NMRA message-set did not deliver the complete earliest equal-time TSO cohort");
         JavaTckSupport.check(timeFive.equals(requesterCallbacks.grants.get(0))
               && publisherTarget.equals(publisherCallbacks.grants.get(0))
               && requesterCallbacks.callbackOrder.equals(
                  Arrays.asList("interaction", "interaction", "grant"))
               && timeFive.equals(requester.queryLogicalTime()),
            "NMRA message-set did not deliver the first cohort before granting at its time");

         requester.nextMessageRequestAvailable(requesterTarget);
         await(requester, () -> requesterCallbacks.timestamped.size() >= 3
               && requesterCallbacks.grants.size() >= 2,
            "NMRA message-set remaining timestamp-8 interaction");
         JavaTckSupport.check(requesterCallbacks.receiveOrder.size() == 2
               && requesterCallbacks.timestamped.size() == 3
               && requesterCallbacks.grants.size() == 2,
            "NMRA message-set duplicated or omitted input on the second request");
         InteractionRecord remaining = requesterCallbacks.timestamped.get(2);
         checkInteraction(remaining, requesterInteraction, requesterParameter,
            publisherHandle, payload);
         JavaTckSupport.check(Arrays.equals(remaining.tag, new byte[] {0x30})
               && timeEight.equals(remaining.time)
               && remaining.receivedOrder == OrderType.TIMESTAMP
               && timeEight.equals(requesterCallbacks.grants.get(1))
               && timeEight.equals(requester.queryLogicalTime())
               && requesterCallbacks.callbackOrder.equals(Arrays.asList(
                  "interaction", "interaction", "grant", "interaction", "grant")),
            "NMRA message-set did not retire the first cohort before the second grant");

         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         requester.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
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

   private static void checkInteraction(InteractionRecord event,
         InteractionClassHandle interaction, ParameterHandle parameter,
         FederateHandle publisher, byte[] payload) {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.values.size() == 1
            && Arrays.equals(event.values.get(parameter), payload)
            && publisher.equals(event.producer),
         "NMRA message-set callback changed the interaction, parameter, or producer");
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

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
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
      private final OrderType receivedOrder;

      private InteractionRecord(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         values = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = arguments.length == 10 ? (LogicalTime<?, ?>) arguments[6] : null;
         receivedOrder = arguments.length == 10 ? (OrderType) arguments[8] : null;
      }
   }

   private static final class Recorder {
      private volatile int timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeRegulationEnabled;
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
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  receiveOrder.add(new InteractionRecord(arguments));
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timestamped.add(new InteractionRecord(arguments));
                  callbackOrder.add("interaction");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK NMRA message-set callbacks";
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
