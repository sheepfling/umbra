package org.hla.rti.tck;

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

/** Covers the empty-input NMR/NMRA grant and actual-lookahead Send Interaction floor. */
final class NoPregrantTsoFloorTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private NoPregrantTsoFloorTck() { }

   static void runNmr(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, timeImplementation, interactionName, parameterName,
         callbackModel, false);
   }

   static void runNmra(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, timeImplementation, interactionName, parameterName,
         callbackModel, true);
   }

   private static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel,
         boolean available) throws Exception {
      String requestKind = available ? "NMRA" : "NMR";
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder peerCallbacks = new Recorder();
      String federation = "java-tck-" + requestKind.toLowerCase()
         + "-no-pregrant-tso-floor-" + UUID.randomUUID();
      boolean requesterConnected = false, peerConnected = false;
      boolean federationCreated = false;
      boolean requesterJoined = false, peerJoined = false;
      try {
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         peer.connect(peerCallbacks.proxy(), callbackModel);
         peerConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         requester.joinFederationExecution("java-tck-no-pregrant-requester", federation);
         requesterJoined = true;
         peer.joinFederationExecution("java-tck-no-pregrant-peer", federation);
         peerJoined = true;

         InteractionClassHandle interaction =
            requester.getInteractionClassHandle(interactionName);
         ParameterHandle parameter = requester.getParameterHandle(interaction, parameterName);
         JavaTckSupport.check(interaction != null && parameter != null,
            requestKind + " no-pregrant adapter FOM did not resolve the interaction handles");
         requester.publishInteractionClass(interaction);
         requester.changeInteractionOrderType(interaction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> peerFactory = peer.getTimeFactory();
         JavaTckSupport.check(requesterFactory.getName().equals(peerFactory.getName()),
            requestKind + " no-pregrant members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> peerInitial = peer.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(peerInitial),
            requestKind + " no-pregrant members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();
         LogicalTimeInterval peerEpsilon =
            (LogicalTimeInterval) peerFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(requester, callbackModel,
            () -> requesterCallbacks.timeConstrainedEnabled,
            requestKind + " no-pregrant requester Time Constrained callback");
         requester.enableTimeRegulation(requesterEpsilon);
         await(requester, callbackModel,
            () -> requesterCallbacks.timeRegulationEnabled != null,
            requestKind + " no-pregrant requester Time Regulation callback");
         peer.enableTimeRegulation(peerEpsilon);
         await(peer, callbackModel,
            () -> peerCallbacks.timeRegulationEnabled != null,
            requestKind + " no-pregrant peer Time Regulation callback");

         LogicalTimeInterval actualLookahead = requester.queryLookahead();
         JavaTckSupport.check(requesterEpsilon.equals(actualLookahead),
            requestKind + " no-pregrant requester did not retain its enabled epsilon lookahead");
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> peerTarget = offset(peerInitial, peerEpsilon, available ? 12 : 10);
         JavaTckSupport.check(requesterCallbacks.grants.isEmpty()
               && requesterCallbacks.receivedInteractionCount == 0,
            requestKind + " no-pregrant requester received a grant or interaction before its request");

         if (!available) {
            requester.nextMessageRequest(requestTime);
            JavaTckSupport.check(requesterCallbacks.grants.isEmpty(),
               "NMR without pre-grant TSO completed before the peer advanced");
         }
         peer.timeAdvanceRequest(peerTarget);
         await(peer, callbackModel,
            () -> peerCallbacks.grants.size() == 1,
            requestKind + " no-pregrant peer Time Advance Grant");
         JavaTckSupport.check(peerCallbacks.grants.size() == 1
               && peerTarget.equals(peerCallbacks.grants.get(0)),
            requestKind + " no-pregrant peer did not advance to its requested target");
         if (available) {
            requester.nextMessageRequestAvailable(requestTime);
         }
         await(requester, callbackModel,
            () -> requesterCallbacks.grants.size() == 1,
            requestKind + " no-pregrant requester Time Advance Grant");
         JavaTckSupport.check(requesterCallbacks.grants.size() == 1
               && requestTime.equals(requesterCallbacks.grants.get(0)),
            requestKind + " without pre-grant TSO did not grant at its request time");
         JavaTckSupport.check(requesterCallbacks.receivedInteractionCount == 0,
            requestKind + " no-pregrant requester received an unexpected timestamped interaction");
         JavaTckSupport.check(requestTime.equals(requester.queryLogicalTime()),
            requestKind + " no-pregrant logical-time query returned the wrong time");

         LogicalTime<?, ?> legalTime = offset(requestTime, actualLookahead, 1);
         byte[] payload = {0x4e, 0x4d, (byte) (available ? 0x41 : 0x52)};
         byte[] tag = {0x4e, 0x4f, 0x50, 0x52, 0x45};
         ParameterHandleValueMap parameters =
            requester.getParameterHandleValueMapFactory().create(1);
         parameters.put(parameter, payload.clone());
         JavaTckSupport.expectFailure(
            () -> requester.sendInteraction(interaction, parameters, tag.clone(), requestTime),
            "InvalidLogicalTime",
            requestKind + " Send Interaction at request time below actual-lookahead floor");
         MessageRetractionReturn accepted = requester.sendInteraction(
            interaction, parameters, tag.clone(), legalTime);
         JavaTckSupport.check(accepted != null && accepted.retractionHandleIsValid
               && accepted.handle != null,
            requestKind + " Send Interaction at request time plus actual lookahead returned no valid handle");

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
         if (model == CallbackModel.HLA_EVOKED) {
            ambassador.evokeCallback(0.0);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      private volatile boolean timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeRegulationEnabled;
      private volatile int receivedInteractionCount;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("timeAdvanceGrant".equals(name)) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name)) {
                  ++receivedInteractionCount;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK no-pregrant callbacks";
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
