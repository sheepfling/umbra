package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks the NMR received-TSO floor for timestamped Update Attribute Values. */
final class NmrReceivedTsoAttributeUpdateFloorTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private NmrReceivedTsoAttributeUpdateFloorTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder peerCallbacks = new Recorder();
      String federation = "java-tck-nmr-received-tso-attribute-floor-" + UUID.randomUUID();
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
         requester.joinFederationExecution("java-tck-nmr-attribute-requester", federation);
         requesterJoined = true;
         peer.joinFederationExecution("java-tck-nmr-attribute-peer", federation);
         peerJoined = true;

         InteractionClassHandle requesterInteraction =
            requester.getInteractionClassHandle(interactionName);
         InteractionClassHandle peerInteraction = peer.getInteractionClassHandle(interactionName);
         ParameterHandle requesterParameter =
            requester.getParameterHandle(requesterInteraction, parameterName);
         ParameterHandle peerParameter = peer.getParameterHandle(peerInteraction, parameterName);
         JavaTckSupport.check(requesterInteraction != null && peerInteraction != null
               && requesterParameter != null && peerParameter != null,
            "NMR received-TSO attribute adapter FOM did not resolve interaction handles");
         JavaTckSupport.check(requesterInteraction.equals(peerInteraction)
               && requesterParameter.equals(peerParameter),
            "NMR received-TSO attribute interaction handles differed across members");
         requester.subscribeInteractionClass(requesterInteraction);
         peer.publishInteractionClass(peerInteraction);
         peer.changeInteractionOrderType(peerInteraction, OrderType.TIMESTAMP);

         hla.rti1516_2025.ObjectClassHandle objectClass =
            requester.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = requester.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null,
            "NMR received-TSO attribute adapter FOM did not resolve object handles");
         AttributeHandleSet attributes = requester.getAttributeHandleSetFactory().create();
         attributes.add(attribute);
         requester.publishObjectClassAttributes(objectClass, attributes);
         hla.rti1516_2025.ObjectInstanceHandle object =
            requester.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null,
            "NMR received-TSO attribute registration returned no object handle");
         requester.changeAttributeOrderType(object, attributes, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> requesterTimeFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> peerTimeFactory = peer.getTimeFactory();
         JavaTckSupport.check(requesterTimeFactory.getName().equals(peerTimeFactory.getName()),
            "NMR received-TSO attribute members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> peerInitial = peer.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(peerInitial),
            "NMR received-TSO attribute members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterTimeFactory.makeEpsilon();
         LogicalTimeInterval peerEpsilon =
            (LogicalTimeInterval) peerTimeFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(requester, callbackModel, () -> requesterCallbacks.timeConstrainedEnabled,
            "NMR received-TSO attribute requester Time Constrained callback");
         requester.enableTimeRegulation(requesterEpsilon);
         await(requester, callbackModel, () -> requesterCallbacks.timeRegulationEnabled != null,
            "NMR received-TSO attribute requester Time Regulation callback");
         peer.enableTimeRegulation(peerEpsilon);
         await(peer, callbackModel, () -> peerCallbacks.timeRegulationEnabled != null,
            "NMR received-TSO attribute peer Time Regulation callback");

         LogicalTimeInterval actualLookahead = requester.queryLookahead();
         JavaTckSupport.check(requesterEpsilon.equals(actualLookahead),
            "NMR received-TSO attribute requester did not retain its enabled epsilon lookahead");
         LogicalTime<?, ?> messageTime = offset(peerInitial, peerEpsilon, 5);
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> peerTarget = offset(peerInitial, peerEpsilon, 4);
         byte[] incomingTag = {0x4e, 0x4d, 0x52, 0x2d, 0x41};
         ParameterHandleValueMap incomingParameters =
            peer.getParameterHandleValueMapFactory().create(1);
         incomingParameters.put(peerParameter, new byte[] {0x4e, 0x4d, 0x52});
         MessageRetractionReturn incomingRetraction = peer.sendInteraction(
            peerInteraction, incomingParameters, incomingTag.clone(), messageTime);
         JavaTckSupport.check(incomingRetraction != null
               && incomingRetraction.retractionHandleIsValid
               && incomingRetraction.handle != null,
            "NMR received-TSO attribute input returned an invalid retraction handle");
         JavaTckSupport.check(requesterCallbacks.interactionTimes.isEmpty(),
            "NMR received-TSO attribute input was delivered before the request");

         requester.nextMessageRequest(requestTime);
         JavaTckSupport.check(requesterCallbacks.grants.isEmpty(),
            "NMR received-TSO attribute requester granted before the peer advanced");
         peer.timeAdvanceRequest(peerTarget);
         awaitBoth(requester, peer, callbackModel,
            () -> requesterCallbacks.interactionTimes.size() == 1
               && requesterCallbacks.grants.size() == 1
               && peerCallbacks.grants.size() == 1,
            "NMR received-TSO attribute delivery and grant");
         JavaTckSupport.check(peerTarget.equals(peerCallbacks.grants.get(0)),
            "NMR received-TSO attribute peer did not advance to its requested bound");
         JavaTckSupport.check(messageTime.equals(requesterCallbacks.interactionTimes.get(0))
               && messageTime.equals(requesterCallbacks.grants.get(0)),
            "NMR received-TSO attribute setup did not deliver and grant at the message time");
         JavaTckSupport.check(messageTime.equals(requester.queryLogicalTime()),
            "NMR received-TSO attribute query returned the wrong grant time");

         byte[] value = {0x4e, 0x4d, 0x52, 0x2d, 0x41};
         AttributeHandleValueMap values = requester.getAttributeHandleValueMapFactory().create(1);
         values.put(attribute, value.clone());
         JavaTckSupport.expectFailure(
            () -> requester.updateAttributeValues(
               object, values, incomingTag.clone(), messageTime),
            "InvalidLogicalTime",
            "NMR received-TSO timestamped attribute update at grant-time floor");

         LogicalTime<?, ?> legalTime = offset(messageTime, actualLookahead, 1);
         MessageRetractionReturn updateRetraction = requester.updateAttributeValues(
            object, values, incomingTag.clone(), legalTime);
         JavaTckSupport.check(updateRetraction != null
               && updateRetraction.retractionHandleIsValid
               && updateRetraction.handle != null,
            "NMR received-TSO timestamped attribute update above the actual-lookahead floor returned no valid handle");

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
                     return "Java TCK NMR received-TSO attribute callbacks";
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
