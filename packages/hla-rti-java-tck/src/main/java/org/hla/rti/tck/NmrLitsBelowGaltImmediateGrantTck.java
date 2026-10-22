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
import java.util.function.BooleanSupplier;

/** Checks NMR delivery and grant at queued LITS strictly below GALT. */
final class NmrLitsBelowGaltImmediateGrantTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;
   private static final long EARLY_CALLBACK_WINDOW_NANOS = 50_000_000L;

   private NmrLitsBelowGaltImmediateGrantTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder regulatorCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-nmr-lits-below-galt-" + UUID.randomUUID();
      boolean regulatorConnected = false;
      boolean requesterConnected = false;
      boolean federationCreated = false;
      boolean regulatorJoined = false;
      boolean requesterJoined = false;
      try {
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         regulatorConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         regulator.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         FederateHandle regulatorHandle = regulator.joinFederationExecution(
            "java-tck-nmr-lits-regulator", federation);
         regulatorJoined = true;
         requester.joinFederationExecution("java-tck-nmr-lits-requester", federation);
         requesterJoined = true;

         InteractionClassHandle regulatorInteraction =
            regulator.getInteractionClassHandle(interactionName);
         InteractionClassHandle requesterInteraction =
            requester.getInteractionClassHandle(interactionName);
         ParameterHandle regulatorParameter =
            regulator.getParameterHandle(regulatorInteraction, parameterName);
         ParameterHandle requesterParameter =
            requester.getParameterHandle(requesterInteraction, parameterName);
         JavaTckSupport.check(regulatorInteraction != null && requesterInteraction != null
               && regulatorParameter != null && requesterParameter != null
               && regulatorInteraction.equals(requesterInteraction)
               && regulatorParameter.equals(requesterParameter),
            "NMR queued-LITS adapter FOM did not resolve stable interaction handles");
         regulator.publishInteractionClass(regulatorInteraction);
         regulator.changeInteractionOrderType(regulatorInteraction, OrderType.TIMESTAMP);
         requester.subscribeInteractionClass(requesterInteraction);

         LogicalTimeFactory<?, ?> regulatorTimeFactory = regulator.getTimeFactory();
         LogicalTimeFactory<?, ?> requesterTimeFactory = requester.getTimeFactory();
         JavaTckSupport.check(regulatorTimeFactory.getName().equals(requesterTimeFactory.getName()),
            "NMR queued-LITS members selected different logical-time factories");
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         JavaTckSupport.check(regulatorInitial.equals(requesterInitial),
            "NMR queued-LITS members began at different logical times");
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorTimeFactory.makeEpsilon();
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterTimeFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(requester, callbackModel, () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "NMR queued-LITS requester Time Constrained callback");
         regulator.enableTimeRegulation(regulatorEpsilon);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.timeRegulationEnabled != null,
            "NMR queued-LITS regulator Time Regulation callback");
         JavaTckSupport.check(regulatorEpsilon.equals(regulator.queryLookahead()),
            "NMR queued-LITS regulator did not retain its configured lookahead");

         LogicalTime<?, ?> messageTime = offset(regulatorInitial, regulatorEpsilon, 3);
         LogicalTime<?, ?> regulatorTarget = offset(regulatorInitial, regulatorEpsilon, 4);
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> expectedGalt = offset(requesterInitial, requesterEpsilon, 5);
         byte[] payload = {0x4c, 0x49, 0x54, 0x53};
         byte[] tag = {0x4e, 0x4d, 0x52};
         ParameterHandleValueMap values =
            regulator.getParameterHandleValueMapFactory().create(1);
         values.put(regulatorParameter, payload.clone());
         MessageRetractionReturn retraction = regulator.sendInteraction(
            regulatorInteraction, values, tag.clone(), messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "NMR queued-LITS send returned no valid message retraction handle");

         regulator.timeAdvanceRequest(regulatorTarget);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.grants.size() == 1,
            "NMR queued-LITS regulator advance");
         JavaTckSupport.check(regulatorTarget.equals(regulatorCallbacks.grants.get(0)),
            "NMR queued-LITS regulator did not reach its requested time");
         JavaTckSupport.check(lessThan(messageTime, expectedGalt),
            "NMR queued-LITS message timestamp was not below the regulator advance bound");
         pumpUnexpectedCallbacks(requester, callbackModel, requesterCallbacks);
         JavaTckSupport.check(requesterCallbacks.interactions.isEmpty()
               && requesterCallbacks.grants.isEmpty(),
            "NMR queued-LITS input was delivered before the request");

         requester.nextMessageRequest(requestTime);
         await(requester, callbackModel,
            () -> requesterCallbacks.interactions.size() == 1
               && requesterCallbacks.grants.size() == 1,
            "NMR queued-LITS delivery and grant");
         InteractionRecord received = requesterCallbacks.interactions.get(0);
         JavaTckSupport.check(received.interaction.equals(requesterInteraction)
               && received.values.size() == 1
               && Arrays.equals(received.values.get(requesterParameter), payload),
            "NMR queued-LITS delivered the wrong interaction or parameter value");
         JavaTckSupport.check(Arrays.equals(received.tag, tag)
               && regulatorHandle.equals(received.producer),
            "NMR queued-LITS callback changed its tag or producer");
         JavaTckSupport.check(messageTime.equals(received.time)
               && received.sentOrder == OrderType.TIMESTAMP
               && received.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(received.retraction),
            "NMR queued-LITS callback changed the TSO timestamp or metadata");
         JavaTckSupport.check(messageTime.equals(requesterCallbacks.grants.get(0)),
            "NMR queued-LITS did not grant exactly at the queued message time");
         JavaTckSupport.check(requesterCallbacks.callbackOrder.equals(
               Arrays.asList("interaction", "grant")),
            "NMR queued-LITS granted before delivering its queued interaction");
         JavaTckSupport.check(messageTime.equals(requester.queryLogicalTime()),
            "NMR queued-LITS requester query returned the wrong logical time");
         JavaTckSupport.check(regulatorTarget.equals(regulator.queryLogicalTime())
               && regulatorCallbacks.grants.size() == 1,
            "NMR queued-LITS required another regulator advance");

         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         regulator.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (requesterJoined) resign(requester);
         if (regulatorJoined) resign(regulator);
         if (federationCreated) {
            try { regulator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (regulatorConnected) disconnect(regulator);
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

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static boolean lessThan(LogicalTime left, LogicalTime right) {
      return left.compareTo(right) < 0;
   }

   private static void pumpUnexpectedCallbacks(RTIambassador ambassador, CallbackModel model,
         Recorder callbacks) throws Exception {
      long deadline = System.nanoTime() + EARLY_CALLBACK_WINDOW_NANOS;
      while (callbacks.interactions.isEmpty() && callbacks.grants.isEmpty()
            && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
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
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile int timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeRegulationEnabled;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<InteractionRecord> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  callbackOrder.add("grant");
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  callbackOrder.add("interaction");
                  interactions.add(new InteractionRecord(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK NMR queued-LITS callbacks";
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
