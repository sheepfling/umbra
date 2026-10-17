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

/** Checks an NMR grant at a received TSO timestamp and its actual-lookahead send floor. */
final class NmrReceivedTsoFloorTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private NmrReceivedTsoFloorTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder peerCallbacks = new Recorder();
      String federation = "java-tck-nmr-received-tso-floor-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean peerConnected = false;
      boolean federationCreated = false;
      boolean requesterJoined = false;
      boolean peerJoined = false;
      try {
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         peer.connect(peerCallbacks.proxy(), callbackModel);
         peerConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         requester.joinFederationExecution("java-tck-nmr-received-requester", federation);
         requesterJoined = true;
         FederateHandle peerHandle = peer.joinFederationExecution(
            "java-tck-nmr-received-peer", federation);
         peerJoined = true;

         InteractionClassHandle requesterInteraction =
            requester.getInteractionClassHandle(interactionName);
         InteractionClassHandle peerInteraction = peer.getInteractionClassHandle(interactionName);
         ParameterHandle requesterParameter =
            requester.getParameterHandle(requesterInteraction, parameterName);
         ParameterHandle peerParameter = peer.getParameterHandle(peerInteraction, parameterName);
         JavaTckSupport.check(requesterInteraction != null && peerInteraction != null
               && requesterParameter != null && peerParameter != null,
            "NMR received-TSO adapter FOM did not resolve the interaction handles");
         JavaTckSupport.check(requesterInteraction.equals(peerInteraction)
               && requesterParameter.equals(peerParameter),
            "NMR received-TSO handles did not retain identity across members");
         requester.publishInteractionClass(requesterInteraction);
         requester.changeInteractionOrderType(requesterInteraction, OrderType.TIMESTAMP);
         requester.subscribeInteractionClass(requesterInteraction);
         peer.publishInteractionClass(peerInteraction);
         peer.changeInteractionOrderType(peerInteraction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> requesterTimeFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> peerTimeFactory = peer.getTimeFactory();
         JavaTckSupport.check(requesterTimeFactory.getName().equals(peerTimeFactory.getName()),
            "NMR received-TSO members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> peerInitial = peer.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(peerInitial),
            "NMR received-TSO members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterTimeFactory.makeEpsilon();
         LogicalTimeInterval peerEpsilon =
            (LogicalTimeInterval) peerTimeFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(requester, callbackModel, () -> requesterCallbacks.timeConstrainedEnabled,
            "NMR received-TSO requester Time Constrained callback");
         requester.enableTimeRegulation(requesterEpsilon);
         await(requester, callbackModel, () -> requesterCallbacks.timeRegulationEnabled != null,
            "NMR received-TSO requester Time Regulation callback");
         peer.enableTimeRegulation(peerEpsilon);
         await(peer, callbackModel, () -> peerCallbacks.timeRegulationEnabled != null,
            "NMR received-TSO peer Time Regulation callback");

         LogicalTimeInterval actualLookahead = requester.queryLookahead();
         JavaTckSupport.check(requesterEpsilon.equals(actualLookahead),
            "NMR received-TSO requester did not retain its enabled epsilon lookahead");
         LogicalTime<?, ?> messageTime = offset(peerInitial, peerEpsilon, 5);
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> peerTarget = offset(peerInitial, peerEpsilon, 4);
         byte[] incomingBytes = {0x4e, 0x4d, 0x52};
         byte[] incomingTag = {0x4e, 0x4d, 0x52, 0x2d, 0x46};
         ParameterHandleValueMap incomingParameters =
            peer.getParameterHandleValueMapFactory().create(1);
         incomingParameters.put(peerParameter, incomingBytes.clone());
         MessageRetractionReturn incomingRetraction = peer.sendInteraction(
            peerInteraction, incomingParameters, incomingTag.clone(), messageTime);
         JavaTckSupport.check(incomingRetraction != null
               && incomingRetraction.retractionHandleIsValid
               && incomingRetraction.handle != null,
            "NMR received-TSO input returned an invalid retraction handle");
         JavaTckSupport.check(requesterCallbacks.interactions.isEmpty(),
            "NMR received-TSO input was delivered before the request");

         requester.nextMessageRequest(requestTime);
         JavaTckSupport.check(requesterCallbacks.grants.isEmpty(),
            "NMR received-TSO requester granted before the regulator advanced");
         peer.timeAdvanceRequest(peerTarget);
         awaitBoth(requester, peer, callbackModel,
            () -> peerCallbacks.grants.size() >= 1
               && requesterCallbacks.grants.size() >= 1
               && requesterCallbacks.interactions.size() >= 1,
            "NMR received-TSO delivery and grant");

         JavaTckSupport.check(peerCallbacks.grants.size() == 1
               && peerTarget.equals(peerCallbacks.grants.get(0)),
            "NMR received-TSO peer did not advance to its requested bound");
         JavaTckSupport.check(requesterCallbacks.interactions.size() == 1
               && requesterCallbacks.grants.size() == 1,
            "NMR received-TSO requester received the wrong callback counts");
         InteractionRecord received = requesterCallbacks.interactions.get(0);
         JavaTckSupport.check(received.interaction.equals(requesterInteraction)
               && received.values.size() == 1
               && Arrays.equals(received.values.get(requesterParameter), incomingBytes),
            "NMR received-TSO callback contained the wrong interaction or parameter value");
         JavaTckSupport.check(Arrays.equals(received.tag, incomingTag)
               && peerHandle.equals(received.producer),
            "NMR received-TSO callback did not preserve its tag or producer");
         JavaTckSupport.check(messageTime.equals(received.time)
               && received.sentOrder == OrderType.TIMESTAMP
               && received.receivedOrder == OrderType.TIMESTAMP
               && incomingRetraction.handle.equals(received.retraction),
            "NMR received-TSO callback did not preserve its timestamp or metadata");
         JavaTckSupport.check(messageTime.equals(requesterCallbacks.grants.get(0)),
            "NMR with received TSO did not grant at the delivered message time");
         JavaTckSupport.check(messageTime.equals(requester.queryLogicalTime()),
            "NMR received-TSO logical-time query returned the wrong time");

         LogicalTime<?, ?> legalTime = offset(messageTime, actualLookahead, 1);
         byte[] outgoingBytes = {0x4e, 0x4d, 0x52, 0x2d, 0x4f};
         byte[] outgoingTag = {0x4e, 0x4d, 0x52, 0x2d, 0x53};
         ParameterHandleValueMap outgoingParameters =
            requester.getParameterHandleValueMapFactory().create(1);
         outgoingParameters.put(requesterParameter, outgoingBytes.clone());
         JavaTckSupport.expectFailure(
            () -> requester.sendInteraction(
               requesterInteraction, outgoingParameters, outgoingTag.clone(), messageTime),
            "InvalidLogicalTime",
            "NMR Send Interaction at received-message grant time below actual-lookahead floor");
         MessageRetractionReturn outgoingRetraction = requester.sendInteraction(
            requesterInteraction, outgoingParameters, outgoingTag.clone(), legalTime);
         JavaTckSupport.check(outgoingRetraction != null
               && outgoingRetraction.retractionHandleIsValid
               && outgoingRetraction.handle != null,
            "NMR Send Interaction at grant time plus actual lookahead returned no valid handle");

         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         peer.resignFederationExecution(ResignAction.NO_ACTION);
         peerJoined = false;
         requester.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (peerJoined) {
            try { peer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (federationCreated) {
            try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); } catch (Exception ignored) { }
         }
         if (peerConnected) {
            try { peer.disconnect(); } catch (Exception ignored) { }
         }
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

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.0);
            second.evokeCallback(0.0);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
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
      private volatile boolean timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeRegulationEnabled;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<InteractionRecord> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  interactions.add(new InteractionRecord(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK NMR received-TSO callbacks";
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
