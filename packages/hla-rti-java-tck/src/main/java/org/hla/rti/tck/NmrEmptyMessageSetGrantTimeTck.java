package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
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

/** Checks the requested-time grant when NMR has no TSO message to deliver. */
final class NmrEmptyMessageSetGrantTimeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private NmrEmptyMessageSetGrantTimeTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder peerCallbacks = new Recorder();
      String federation = "java-tck-nmr-empty-message-set-" + UUID.randomUUID();
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
         requester.joinFederationExecution("java-tck-nmr-empty-requester", federation);
         requesterJoined = true;
         peer.joinFederationExecution("java-tck-nmr-empty-peer", federation);
         peerJoined = true;

         LogicalTimeFactory<?, ?> requesterTimeFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> peerTimeFactory = peer.getTimeFactory();
         JavaTckSupport.check(requesterTimeFactory.getName().equals(peerTimeFactory.getName()),
            "NMR empty-message members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> peerInitial = peer.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(peerInitial),
            "NMR empty-message members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterTimeFactory.makeEpsilon();
         LogicalTimeInterval peerEpsilon =
            (LogicalTimeInterval) peerTimeFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(requester, callbackModel, () -> requesterCallbacks.timeConstrainedEnabled,
            "NMR empty-message requester Time Constrained callback");
         requester.enableTimeRegulation(requesterEpsilon);
         await(requester, callbackModel, () -> requesterCallbacks.timeRegulationEnabled != null,
            "NMR empty-message requester Time Regulation callback");
         peer.enableTimeRegulation(peerEpsilon);
         await(peer, callbackModel, () -> peerCallbacks.timeRegulationEnabled != null,
            "NMR empty-message peer Time Regulation callback");

         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> peerTarget = offset(peerInitial, peerEpsilon, 10);
         JavaTckSupport.check(requestTime.equals(peerTarget),
            "NMR empty-message request and peer advance targets differ");
         requester.nextMessageRequest(requestTime);
         JavaTckSupport.check(requesterCallbacks.grants.isEmpty(),
            "NMR empty-message requester granted before the peer advanced");
         peer.timeAdvanceRequest(peerTarget);
         awaitBoth(requester, peer, callbackModel,
            () -> requesterCallbacks.grants.size() == 1 && peerCallbacks.grants.size() == 1,
            "NMR empty-message peer and requester grants");

         JavaTckSupport.check(requesterCallbacks.grants.size() == 1
               && requestTime.equals(requesterCallbacks.grants.get(0)),
            "NMR with no delivered TSO did not grant exactly once at the requested logical time");
         JavaTckSupport.check(peerCallbacks.grants.size() == 1
               && peerTarget.equals(peerCallbacks.grants.get(0)),
            "NMR empty-message peer did not reach its requested advance time");
         JavaTckSupport.check(requesterCallbacks.interactionTimes.isEmpty(),
            "NMR empty-message grant unexpectedly delivered a timestamped interaction");
         JavaTckSupport.check(requestTime.equals(requester.queryLogicalTime()),
            "NMR empty-message query returned the wrong logical time");

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
                  if ("toString".equals(name)) return "Java TCK NMR empty-message callbacks";
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
