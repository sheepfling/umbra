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
import hla.rti1516_2025.TimeQueryReturn;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Covers TARA delivery at LITS under undefined GALT with both NRG switch values. */
final class TaraUndefinedGaltNrgTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long QUIET_OBSERVATION_NANOS = 250_000_000L;

   private TaraUndefinedGaltNrgTck() { }

   static void run(RtiFactory factory, String nrgDisabledFom, String nrgEnabledFom,
         String timeImplementation, String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      runOne(factory, nrgDisabledFom, timeImplementation, interactionClassName,
         parameterName, callbackModel, false);
      runOne(factory, nrgEnabledFom, timeImplementation, interactionClassName,
         parameterName, callbackModel, true);
   }

   static void runNoTso(RtiFactory factory, String nrgDisabledFom, String nrgEnabledFom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      runNoTsoOne(factory, nrgDisabledFom, timeImplementation, callbackModel, false);
      runNoTsoOne(factory, nrgEnabledFom, timeImplementation, callbackModel, true);
   }

   private static void runNoTsoOne(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel, boolean expectedNrgEnabled)
         throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder regulatorCallbacks = new Recorder();
      String federation = "java-tck-tara-no-tso-undefined-galt-"
         + (expectedNrgEnabled ? "nrg-enabled-" : "nrg-disabled-") + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean regulatorConnected = false;
      boolean federationCreated = false;
      boolean requesterJoined = false;
      boolean regulatorJoined = false;
      boolean requesterConstrained = false;
      boolean regulatorRegulating = false;
      try {
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         regulatorConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         requester.joinFederationExecution("java-tck-tara-no-tso-requester", federation);
         requesterJoined = true;
         regulator.joinFederationExecution("java-tck-tara-no-tso-regulator", federation);
         regulatorJoined = true;

         boolean nrgEnabled = requester.getNonRegulatedGrantSwitch();
         JavaTckSupport.check(nrgEnabled == expectedNrgEnabled
               && regulator.getNonRegulatedGrantSwitch() == nrgEnabled,
            "No-TSO TARA adapter FOM switch did not match the expected NRG setting");
         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         JavaTckSupport.check(requesterFactory.getName().equals(regulatorFactory.getName()),
            "No-TSO TARA members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(regulatorInitial),
            "No-TSO TARA members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorFactory.makeEpsilon();

         requester.enableTimeConstrained();
         requesterConstrained = true;
         await(requester, callbackModel,
            () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "No-TSO TARA requester Time Constrained callback");
         regulator.enableTimeRegulation(regulatorEpsilon);
         regulatorRegulating = true;
         await(regulator, callbackModel,
            () -> regulatorCallbacks.timeRegulationEnabled == 1,
            "No-TSO TARA regulator Time Regulation callback");
         regulator.disableTimeRegulation();
         regulatorRegulating = false;
         settleCallbacks(requester, regulator, callbackModel, QUIET_OBSERVATION_NANOS);

         TimeQueryReturn galt = requester.queryGALT();
         JavaTckSupport.check(galt != null && !galt.timeIsValid,
            "No-TSO TARA setup unexpectedly had a defined GALT");
         JavaTckSupport.check(requesterCallbacks.interactions.isEmpty()
               && requesterCallbacks.grants.isEmpty(),
            "No-TSO TARA setup unexpectedly delivered an interaction or grant");
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         requester.timeAdvanceRequestAvailable(requestTime);

         if (!expectedNrgEnabled) {
            settleCallbacks(requester, regulator, callbackModel, QUIET_OBSERVATION_NANOS);
            JavaTckSupport.check(requesterCallbacks.interactions.isEmpty()
                  && requesterCallbacks.grants.isEmpty()
                  && requester.queryLogicalTime().equals(requesterInitial),
               "No-TSO TARA with undefined GALT and NRG disabled advanced before regulation resumed");

            regulator.enableTimeRegulation(regulatorEpsilon);
            regulatorRegulating = true;
            await(regulator, callbackModel,
               () -> regulatorCallbacks.timeRegulationEnabled == 2,
               "No-TSO TARA resumed regulator Time Regulation callback");
            LogicalTime<?, ?> regulatorTarget = offset(regulatorInitial, regulatorEpsilon, 11);
            regulator.timeAdvanceRequest(regulatorTarget);
            await(regulator, callbackModel,
               () -> regulatorCallbacks.grants.size() == 1,
               "No-TSO TARA resumed regulator advance");
            JavaTckSupport.check(regulatorCallbacks.grants.get(0).equals(regulatorTarget),
               "No-TSO TARA regulator did not reach time 11");
         }

         awaitBoth(requester, regulator, callbackModel,
            () -> requesterCallbacks.grants.size() == 1,
            "No-TSO TARA grant at the requested time");
         JavaTckSupport.check(requesterCallbacks.interactions.isEmpty()
               && requesterCallbacks.grants.size() == 1
               && requesterCallbacks.callbackOrder.equals(Arrays.asList("grant"))
               && requesterCallbacks.grants.get(0).equals(requestTime)
               && requester.queryLogicalTime().equals(requestTime),
            "No-TSO TARA did not grant exactly once at its requested time without an interaction");
         JavaTckSupport.check(regulatorCallbacks.grants.size()
               == (expectedNrgEnabled ? 0 : 1),
            "No-TSO TARA used the wrong number of regulator advances");
         if (expectedNrgEnabled) {
            TimeQueryReturn finalGalt = requester.queryGALT();
            JavaTckSupport.check(finalGalt != null && !finalGalt.timeIsValid,
               "No-TSO TARA NRG-enabled grant unexpectedly defined GALT");
         }

         requester.disableTimeConstrained();
         requesterConstrained = false;
         if (regulatorRegulating) {
            regulator.disableTimeRegulation();
            regulatorRegulating = false;
         }
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         requester.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (requesterConstrained && requesterJoined) {
            try { requester.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulatorRegulating && regulatorJoined) {
            try { regulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (requesterJoined) resign(requester);
         if (regulatorJoined) resign(regulator);
         if (federationCreated) {
            try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (regulatorConnected) disconnect(regulator);
      }
   }

   private static void runOne(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel,
         boolean expectedNrgEnabled) throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder regulatorCallbacks = new Recorder();
      String federation = "java-tck-tara-undefined-galt-"
         + (expectedNrgEnabled ? "nrg-enabled-" : "nrg-disabled-") + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean regulatorConnected = false;
      boolean federationCreated = false;
      boolean requesterJoined = false;
      boolean regulatorJoined = false;
      boolean requesterSubscribed = false;
      boolean regulatorPublished = false;
      boolean requesterConstrained = false;
      boolean regulatorRegulating = false;
      InteractionClassHandle requesterInteraction = null;
      InteractionClassHandle regulatorInteraction = null;
      try {
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         regulatorConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         requester.joinFederationExecution("java-tck-tara-requester", federation);
         requesterJoined = true;
         FederateHandle regulatorHandle = regulator.joinFederationExecution(
            "java-tck-tara-regulator", federation);
         regulatorJoined = true;

         boolean nrgEnabled = requester.getNonRegulatedGrantSwitch();
         JavaTckSupport.check(nrgEnabled == expectedNrgEnabled
               && regulator.getNonRegulatedGrantSwitch() == nrgEnabled,
            "TARA adapter FOM switch did not match the expected NRG setting");
         requesterInteraction = requester.getInteractionClassHandle(interactionClassName);
         regulatorInteraction = regulator.getInteractionClassHandle(interactionClassName);
         ParameterHandle requesterParameter = requester.getParameterHandle(
            requesterInteraction, parameterName);
         ParameterHandle regulatorParameter = regulator.getParameterHandle(
            regulatorInteraction, parameterName);
         JavaTckSupport.check(requesterInteraction != null && regulatorInteraction != null
               && requesterParameter != null && regulatorParameter != null
               && requesterInteraction.equals(regulatorInteraction)
               && requesterParameter.equals(regulatorParameter),
            "TARA undefined-GALT FOM did not resolve matching interaction handles");
         regulator.publishInteractionClass(regulatorInteraction);
         regulatorPublished = true;
         regulator.changeInteractionOrderType(regulatorInteraction, OrderType.TIMESTAMP);
         requester.subscribeInteractionClass(requesterInteraction);
         requesterSubscribed = true;

         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         JavaTckSupport.check(requesterFactory.getName().equals(regulatorFactory.getName()),
            "TARA undefined-GALT members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(regulatorInitial),
            "TARA undefined-GALT members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorFactory.makeEpsilon();

         requester.enableTimeConstrained();
         requesterConstrained = true;
         await(requester, callbackModel,
            () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "TARA undefined-GALT requester Time Constrained callback");
         regulator.enableTimeRegulation(regulatorEpsilon);
         regulatorRegulating = true;
         await(regulator, callbackModel,
            () -> regulatorCallbacks.timeRegulationEnabled == 1,
            "TARA undefined-GALT regulator Time Regulation callback");

         LogicalTime<?, ?> messageTime = offset(regulatorInitial, regulatorEpsilon, 3);
         LogicalTime<?, ?> regulatorTarget = offset(regulatorInitial, regulatorEpsilon, 4);
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         byte[] payload = {0x54, 0x41, 0x52};
         byte[] tag = {0x54, 0x41, 0x52, 0x41};
         ParameterHandleValueMap values =
            regulator.getParameterHandleValueMapFactory().create(1);
         values.put(regulatorParameter, payload.clone());
         MessageRetractionReturn sent = regulator.sendInteraction(
            regulatorInteraction, values, tag.clone(), messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "TARA undefined-GALT timestamped send returned no valid retraction handle");
         regulator.disableTimeRegulation();
         regulatorRegulating = false;
         settleCallbacks(requester, regulator, callbackModel, QUIET_OBSERVATION_NANOS);

         TimeQueryReturn galt = requester.queryGALT();
         JavaTckSupport.check(galt != null && !galt.timeIsValid,
            "TARA undefined-GALT setup unexpectedly had a defined GALT");
         JavaTckSupport.check(requesterCallbacks.interactions.isEmpty()
               && requesterCallbacks.grants.isEmpty(),
            "TARA undefined-GALT TSO was delivered before the request");
         requester.timeAdvanceRequestAvailable(requestTime);

         if (!expectedNrgEnabled) {
            settleCallbacks(requester, regulator, callbackModel, QUIET_OBSERVATION_NANOS);
            JavaTckSupport.check(requesterCallbacks.interactions.isEmpty()
                  && requesterCallbacks.grants.isEmpty()
                  && requester.queryLogicalTime().equals(requesterInitial),
               "TARA with undefined GALT and NRG disabled advanced before regulation resumed");

            regulator.enableTimeRegulation(regulatorEpsilon);
            regulatorRegulating = true;
            await(regulator, callbackModel,
               () -> regulatorCallbacks.timeRegulationEnabled == 2,
               "TARA undefined-GALT resumed regulator Time Regulation callback");
            regulator.timeAdvanceRequest(regulatorTarget);
            await(regulator, callbackModel,
               () -> regulatorCallbacks.grants.size() == 1,
               "TARA undefined-GALT regulator advance");
            JavaTckSupport.check(regulatorCallbacks.grants.get(0).equals(regulatorTarget),
               "TARA undefined-GALT regulator did not reach time 4");
         }

         awaitBoth(requester, regulator, callbackModel,
            () -> requesterCallbacks.interactions.size() == 1
               && requesterCallbacks.grants.size() == 1,
            "TARA undefined-GALT delivery and grant at LITS");
         JavaTckSupport.check(requesterCallbacks.interactions.size() == 1
               && requesterCallbacks.grants.size() == 1,
            "TARA undefined-GALT delivered the wrong number of TSO messages or grants");
         TimedInteraction received = requesterCallbacks.interactions.get(0);
         JavaTckSupport.check(requesterInteraction.equals(received.interaction)
               && received.parameters.size() == 1
               && Arrays.equals(payload, received.parameters.get(requesterParameter))
               && Arrays.equals(tag, received.tag)
               && regulatorHandle.equals(received.producer)
               && messageTime.equals(received.time)
               && received.sentOrder == OrderType.TIMESTAMP
               && received.receivedOrder == OrderType.TIMESTAMP,
            "TARA undefined-GALT changed the queued interaction or delivery metadata");
         JavaTckSupport.check(requesterCallbacks.callbackOrder.equals(
               Arrays.asList("interaction", "grant"))
               && requesterCallbacks.grants.get(0).equals(messageTime)
               && requester.queryLogicalTime().equals(messageTime),
            "TARA undefined-GALT did not deliver at LITS before granting at time 3");
         JavaTckSupport.check(regulatorCallbacks.grants.size()
               == (expectedNrgEnabled ? 0 : 1),
            "TARA undefined-GALT used the wrong number of regulator advances");

         requester.disableTimeConstrained();
         requesterConstrained = false;
         if (regulatorRegulating) {
            regulator.disableTimeRegulation();
            regulatorRegulating = false;
         }
         requester.unsubscribeInteractionClass(requesterInteraction);
         requesterSubscribed = false;
         regulator.unpublishInteractionClass(regulatorInteraction);
         regulatorPublished = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         requester.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (requesterConstrained && requesterJoined) {
            try { requester.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulatorRegulating && regulatorJoined) {
            try { regulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (requesterSubscribed && requesterJoined && requesterInteraction != null) {
            try { requester.unsubscribeInteractionClass(requesterInteraction); }
            catch (Exception ignored) { }
         }
         if (regulatorPublished && regulatorJoined && regulatorInteraction != null) {
            try { regulator.unpublishInteractionClass(regulatorInteraction); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) resign(requester);
         if (regulatorJoined) resign(regulator);
         if (federationCreated) {
            try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (regulatorConnected) disconnect(regulator);
      }
   }

   private static void settleCallbacks(RTIambassador requester, RTIambassador regulator,
         CallbackModel model, long durationNanos) throws Exception {
      long deadline = System.nanoTime() + durationNanos;
      while (System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            requester.evokeCallback(0.0);
            regulator.evokeCallback(0.0);
         } else {
            Thread.sleep(1L);
         }
      }
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

   private static void awaitBoth(RTIambassador requester, RTIambassador regulator,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            requester.evokeCallback(0.0);
            regulator.evokeCallback(0.0);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
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

      private TimedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
      }
   }

   private static final class Recorder {
      private volatile int timeConstrainedEnabled;
      private volatile int timeRegulationEnabled;
      private final List<TimedInteraction> interactions = new CopyOnWriteArrayList<>();
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
                  timeRegulationEnabled++;
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  interactions.add(new TimedInteraction(arguments));
                  callbackOrder.add("interaction");
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK TARA undefined-GALT callbacks";
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
