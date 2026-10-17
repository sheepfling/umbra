package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks the NMR received-TSO floor for timestamped Send Directed Interaction. */
final class NmrReceivedTsoDirectedInteractionFloorTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private NmrReceivedTsoDirectedInteractionFloorTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String targetObjectClassName, String attributeName, String incomingInteractionName,
         String incomingParameterName, String directedInteractionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder peerCallbacks = new Recorder();
      String federation = "java-tck-nmr-received-tso-directed-floor-" + UUID.randomUUID();
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
         requester.joinFederationExecution("java-tck-nmr-directed-requester", federation);
         requesterJoined = true;
         peer.joinFederationExecution("java-tck-nmr-directed-peer", federation);
         peerJoined = true;

         InteractionClassHandle requesterIncoming =
            requester.getInteractionClassHandle(incomingInteractionName);
         InteractionClassHandle peerIncoming = peer.getInteractionClassHandle(incomingInteractionName);
         ParameterHandle requesterIncomingParameter =
            requester.getParameterHandle(requesterIncoming, incomingParameterName);
         ParameterHandle peerIncomingParameter =
            peer.getParameterHandle(peerIncoming, incomingParameterName);
         InteractionClassHandle requesterDirected =
            requester.getInteractionClassHandle(directedInteractionName);
         InteractionClassHandle peerDirected = peer.getInteractionClassHandle(directedInteractionName);
         JavaTckSupport.check(requesterIncoming != null && peerIncoming != null
               && requesterIncomingParameter != null && peerIncomingParameter != null
               && requesterDirected != null && peerDirected != null,
            "NMR received-TSO directed adapter FOM did not resolve interaction handles");
         JavaTckSupport.check(requesterIncoming.equals(peerIncoming)
               && requesterIncomingParameter.equals(peerIncomingParameter)
               && requesterDirected.equals(peerDirected),
            "NMR received-TSO directed interaction handles differed across members");

         ObjectClassHandle requesterObjectClass =
            requester.getObjectClassHandle(targetObjectClassName);
         ObjectClassHandle peerObjectClass = peer.getObjectClassHandle(targetObjectClassName);
         AttributeHandle requesterAttribute =
            requester.getAttributeHandle(requesterObjectClass, attributeName);
         AttributeHandle peerAttribute = peer.getAttributeHandle(peerObjectClass, attributeName);
         JavaTckSupport.check(requesterObjectClass != null && peerObjectClass != null
               && requesterAttribute != null && peerAttribute != null,
            "NMR received-TSO directed target adapter FOM did not resolve object handles");
         JavaTckSupport.check(requesterObjectClass.equals(peerObjectClass)
               && requesterAttribute.equals(peerAttribute),
            "NMR received-TSO directed target handles differed across members");

         requester.subscribeInteractionClass(requesterIncoming);
         peer.publishInteractionClass(peerIncoming);
         peer.changeInteractionOrderType(peerIncoming, OrderType.TIMESTAMP);
         AttributeHandleSet requesterAttributes =
            requester.getAttributeHandleSetFactory().create();
         requesterAttributes.add(requesterAttribute);
         AttributeHandleSet peerAttributes = peer.getAttributeHandleSetFactory().create();
         peerAttributes.add(peerAttribute);
         requester.publishObjectClassAttributes(requesterObjectClass, requesterAttributes);
         peer.subscribeObjectClassAttributes(peerObjectClass, peerAttributes);
         requester.publishObjectClassDirectedInteractions(requesterObjectClass,
            JavaTckSupport.interactionSet(requester, requesterDirected));
         peer.subscribeObjectClassDirectedInteractions(peerObjectClass,
            JavaTckSupport.interactionSet(peer, peerDirected));
         requester.changeInteractionOrderType(requesterDirected, OrderType.TIMESTAMP);
         ObjectInstanceHandle target = requester.registerObjectInstance(requesterObjectClass);
         JavaTckSupport.check(target != null,
            "NMR received-TSO directed target registration returned no object handle");

         LogicalTimeFactory<?, ?> requesterTimeFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> peerTimeFactory = peer.getTimeFactory();
         JavaTckSupport.check(requesterTimeFactory.getName().equals(peerTimeFactory.getName()),
            "NMR received-TSO directed members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> peerInitial = peer.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(peerInitial),
            "NMR received-TSO directed members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterTimeFactory.makeEpsilon();
         LogicalTimeInterval peerEpsilon =
            (LogicalTimeInterval) peerTimeFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(requester, callbackModel, () -> requesterCallbacks.timeConstrainedEnabled,
            "NMR received-TSO directed requester Time Constrained callback");
         requester.enableTimeRegulation(requesterEpsilon);
         await(requester, callbackModel, () -> requesterCallbacks.timeRegulationEnabled != null,
            "NMR received-TSO directed requester Time Regulation callback");
         peer.enableTimeRegulation(peerEpsilon);
         await(peer, callbackModel, () -> peerCallbacks.timeRegulationEnabled != null,
            "NMR received-TSO directed peer Time Regulation callback");

         LogicalTimeInterval actualLookahead = requester.queryLookahead();
         JavaTckSupport.check(requesterEpsilon.equals(actualLookahead),
            "NMR received-TSO directed requester did not retain its enabled epsilon lookahead");
         LogicalTime<?, ?> messageTime = offset(peerInitial, peerEpsilon, 5);
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> peerTarget = offset(peerInitial, peerEpsilon, 4);
         ParameterHandleValueMap incomingParameters =
            peer.getParameterHandleValueMapFactory().create(1);
         incomingParameters.put(peerIncomingParameter, new byte[] {0x4e, 0x4d, 0x52});
         byte[] incomingTag = {0x4e, 0x4d, 0x52, 0x2d, 0x44};
         MessageRetractionReturn incomingRetraction = peer.sendInteraction(
            peerIncoming, incomingParameters, incomingTag.clone(), messageTime);
         JavaTckSupport.check(incomingRetraction != null
               && incomingRetraction.retractionHandleIsValid
               && incomingRetraction.handle != null,
            "NMR received-TSO directed input returned an invalid retraction handle");
         JavaTckSupport.check(requesterCallbacks.interactionTimes.isEmpty(),
            "NMR received-TSO directed input was delivered before the request");

         requester.nextMessageRequest(requestTime);
         JavaTckSupport.check(requesterCallbacks.grants.isEmpty(),
            "NMR received-TSO directed requester granted before the peer advanced");
         peer.timeAdvanceRequest(peerTarget);
         awaitBoth(requester, peer, callbackModel,
            () -> requesterCallbacks.interactionTimes.size() == 1
               && requesterCallbacks.grants.size() == 1 && peerCallbacks.grants.size() == 1,
            "NMR received-TSO directed delivery and grant");
         JavaTckSupport.check(messageTime.equals(requesterCallbacks.interactionTimes.get(0))
               && messageTime.equals(requesterCallbacks.grants.get(0)),
            "NMR received-TSO directed setup did not deliver and grant at the message time");
         JavaTckSupport.check(messageTime.equals(requester.queryLogicalTime()),
            "NMR received-TSO directed query returned the wrong grant time");

         ParameterHandleValueMap directedParameters =
            requester.getParameterHandleValueMapFactory().create(0);
         byte[] directedTag = {0x4e, 0x4d, 0x52, 0x2d, 0x44};
         JavaTckSupport.expectFailure(
            () -> requester.sendDirectedInteraction(
               requesterDirected, target, directedParameters, directedTag.clone(), messageTime),
            "InvalidLogicalTime",
            "NMR received-TSO timestamped directed interaction at grant-time floor");

         LogicalTime<?, ?> legalTime = offset(messageTime, actualLookahead, 1);
         requester.sendDirectedInteraction(
            requesterDirected, target, directedParameters, directedTag.clone(), legalTime);

         requester.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         requesterJoined = false;
         peer.resignFederationExecution(ResignAction.NO_ACTION);
         peerJoined = false;
         requester.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
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

   private static final class Recorder {
      private volatile boolean timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeRegulationEnabled;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> interactionTimes = new CopyOnWriteArrayList<>();

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
                  interactionTimes.add((LogicalTime<?, ?>) arguments[6]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK NMR received-TSO directed callbacks";
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
