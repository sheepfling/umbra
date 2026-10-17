package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.TimeUnit;

/** Checks timestamp bounds against the requested TAR time while the TAR is pending. */
final class TimestampedAttributeUpdateTimeAdvancingCutoffTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final long PENDING_OBSERVATION_NANOS = 250_000_000L;

   private TimestampedAttributeUpdateTimeAdvancingCutoffTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder senderRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      String federation = "java-tck-attribute-time-advancing-cutoff-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean peerConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean peerJoined = false;
      try {
         sender.connect(senderRecorder.proxy(), callbackModel);
         senderConnected = true;
         peer.connect(peerRecorder.proxy(), callbackModel);
         peerConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         sender.joinFederationExecution("attribute-time-advancing-sender", federation);
         senderJoined = true;
         peer.joinFederationExecution("attribute-time-advancing-peer", federation);
         peerJoined = true;

         ObjectClassHandle objectClass = sender.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = sender.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null,
            "timestamped attribute advancing-cutoff FOM lookup returned a null handle");
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(sender, attribute);
         sender.publishObjectClassAttributes(objectClass, attributes);
         sender.changeDefaultAttributeOrderType(
            objectClass, attributes, hla.rti1516_2025.OrderType.TIMESTAMP);
         ObjectInstanceHandle object = sender.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null,
            "timestamped attribute advancing-cutoff registration returned a null handle");

         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         LogicalTimeFactory<?, ?> peerTimeFactory = peer.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(peerTimeFactory.getName()),
            "timestamped attribute advancing-cutoff members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval peerEpsilon = (LogicalTimeInterval) peerTimeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = epsilon;
         LogicalTimeInterval peerLookahead = peerEpsilon;
         for (int step = 1; step < 5; ++step) {
            lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
            peerLookahead = (LogicalTimeInterval) peerLookahead.add(peerEpsilon);
         }
         LogicalTime initial = sender.queryLogicalTime();
         LogicalTime peerInitial = peer.queryLogicalTime();
         JavaTckSupport.check(initial.equals(peerInitial),
            "timestamped attribute advancing-cutoff members began at different logical times");

         sender.enableTimeConstrained();
         await(callbackModel, sender, "sender Time Constrained callback",
            () -> senderRecorder.timeConstrainedEnabled);
         sender.enableTimeRegulation(lookahead);
         await(callbackModel, sender, "sender Time Regulation callback",
            () -> senderRecorder.timeRegulationEnabled);
         peer.enableTimeRegulation(peerLookahead);
         await(callbackModel, peer, "peer Time Regulation callback",
            () -> peerRecorder.timeRegulationEnabled);

         LogicalTime requestedTime = offset(initial, epsilon, 10);
         LogicalTime belowBound = offset(initial, epsilon, 14);
         LogicalTime equalBound = offset(initial, epsilon, 15);
         LogicalTime aboveBound = offset(initial, epsilon, 16);
         LogicalTime peerRequestedTime = offset(peerInitial, peerEpsilon, 10);
         JavaTckSupport.check(requestedTime.equals(peerRequestedTime),
            "timestamped attribute advancing-cutoff members computed different TAR(10) values");
         sender.timeAdvanceRequest(requestedTime);
         observePending(sender, callbackModel);
         JavaTckSupport.check(senderRecorder.timeAdvanceGrantTimes.isEmpty(),
            "the sender's TAR(10) was not held pending by the peer regulator");

         AttributeHandleValueMap values =
            sender.getAttributeHandleValueMapFactory().create(1);
         values.put(attribute, new byte[] {0x42});
         byte[] tag = new byte[] {0x54, 0x41, 0x52};
         JavaTckSupport.expectFailure(
            () -> sender.updateAttributeValues(object, values, tag, belowBound),
            "InvalidLogicalTime",
            "timestamped attribute update below requested TAR time plus lookahead");
         sender.updateAttributeValues(object, values, tag, equalBound);
         sender.updateAttributeValues(object, values, tag, aboveBound);

         peer.timeAdvanceRequest(peerRequestedTime);
         await(callbackModel, sender, "sender TAR(10) grant",
            () -> senderRecorder.timeAdvanceGrantTimes.size() >= 1);
         await(callbackModel, peer, "peer TAR(10) grant",
            () -> peerRecorder.timeAdvanceGrantTimes.size() >= 1);
         JavaTckSupport.check(senderRecorder.timeAdvanceGrantTimes.size() == 1
               && peerRecorder.timeAdvanceGrantTimes.size() == 1
               && requestedTime.equals(senderRecorder.timeAdvanceGrantTimes.get(0))
               && peerRequestedTime.equals(peerRecorder.timeAdvanceGrantTimes.get(0)),
            "timestamped attribute advancing-cutoff did not grant both federates at TAR(10)");
      } finally {
         if (peerJoined) resign(peer, ResignAction.NO_ACTION);
         if (senderJoined) {
            resign(sender, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         }
         if (created) {
            try {
               sender.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (peerConnected) disconnect(peer);
         if (senderConnected) disconnect(sender);
      }
   }

   private static void observePending(RTIambassador sender, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + PENDING_OBSERVATION_NANOS;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            sender.evokeCallback(0.01);
         } else {
            Thread.sleep(1);
         }
      }
   }

   private static void await(CallbackModel callbackModel, RTIambassador ambassador,
         String description, Check condition) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.test() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            ambassador.evokeCallback(0.01);
         } else {
            Thread.sleep(1);
         }
      }
      JavaTckSupport.check(condition.test(), "timed out waiting for " + description);
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try {
         ambassador.resignFederationExecution(action);
      } catch (Exception ignored) {
      }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
   }

   @FunctionalInterface
   private interface Check {
      boolean test();
   }

   private static final class Recorder {
      private volatile boolean timeConstrainedEnabled;
      private volatile boolean timeRegulationEnabled;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes =
         new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK time-advancing attribute recorder";
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
