package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InvalidLogicalTime;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies the timestamp cutoff after a completed Time Advance Request grant. */
final class TimestampedInteractionTarPostGrantCutoffTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;

   private TimestampedInteractionTarPostGrantCutoffTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador frontier = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      Recorder publisherRecorder = new Recorder();
      Recorder frontierRecorder = new Recorder();
      String federation = "java-tck-tar-post-grant-cutoff-" + UUID.randomUUID();
      boolean receiverConnected = false;
      boolean publisherConnected = false;
      boolean frontierConnected = false;
      boolean created = false;
      boolean receiverJoined = false;
      boolean publisherJoined = false;
      boolean frontierJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean receiverConstrained = false;
      boolean publisherRegulating = false;
      boolean frontierRegulating = false;
      InteractionClassHandle receiverInteraction = null;
      InteractionClassHandle publisherInteraction = null;
      try {
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         frontier.connect(frontierRecorder.proxy(), callbackModel);
         frontierConnected = true;
         receiver.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         receiver.joinFederationExecution("java-tck-tar-cutoff-receiver", federation);
         receiverJoined = true;
         publisher.joinFederationExecution("java-tck-tar-cutoff-late-publisher", federation);
         publisherJoined = true;
         frontier.joinFederationExecution("java-tck-tar-cutoff-frontier", federation);
         frontierJoined = true;

         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         JavaTckSupport.check(receiverInteraction.equals(publisherInteraction)
               && receiverParameter.equals(publisherParameter),
            "post-grant cutoff interaction handles differed across federates");
         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> frontierTimeFactory = frontier.getTimeFactory();
         JavaTckSupport.check(receiverTimeFactory.getName().equals(publisherTimeFactory.getName())
               && receiverTimeFactory.getName().equals(frontierTimeFactory.getName()),
            "post-grant cutoff members selected different logical-time factories");
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverTimeFactory.makeEpsilon();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTimeInterval frontierEpsilon =
            (LogicalTimeInterval) frontierTimeFactory.makeEpsilon();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime frontierInitial = frontier.queryLogicalTime();
         JavaTckSupport.check(receiverInitial.equals(publisherInitial)
               && receiverInitial.equals(frontierInitial),
            "post-grant cutoff members began at different logical times");

         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(callbackModel, receiver, "receiver Time Constrained callback",
            () -> receiverRecorder.timeConstrainedEnabled);
         frontier.enableTimeRegulation(frontierEpsilon);
         frontierRegulating = true;
         await(callbackModel, frontier, "frontier Time Regulation callback",
            () -> frontierRecorder.timeRegulationEnabled);

         LogicalTime receiverAtFive = offset(receiverInitial, receiverEpsilon, 5);
         LogicalTime frontierAtFive = offset(frontierInitial, frontierEpsilon, 5);
         frontier.timeAdvanceRequest(frontierAtFive);
         await(callbackModel, frontier, "frontier TAR(5) grant",
            () -> frontierRecorder.timeAdvanceGrantTimes.size() == 1);
         receiver.timeAdvanceRequest(receiverAtFive);
         await(callbackModel, receiver, "completed receiver TAR(5) grant",
            () -> receiverRecorder.timeAdvanceGrantTimes.size() == 1);
         JavaTckSupport.check(receiverAtFive.equals(receiverRecorder.timeAdvanceGrantTimes.get(0))
               && receiverRecorder.receivedTimes.isEmpty(),
            "receiver did not complete TAR(5) without an interaction delivery");

         // The publisher becomes time-regulated only after the receiver has
         // completed TAR(5), creating late TSO arrivals at timestamps 4 and 5.
         publisher.enableTimeRegulation(publisherEpsilon);
         publisherRegulating = true;
         await(callbackModel, publisher, "late publisher Time Regulation callback",
            () -> publisherRecorder.timeRegulationEnabled);

         byte[] payload = new byte[] {0x47, 0x52, 0x41, 0x4e, 0x54};
         byte[] belowTag = new byte[] {'b', 'e', 'l', 'o', 'w'};
         byte[] equalTag = new byte[] {'e', 'q', 'u', 'a', 'l'};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload);
         LogicalTime publisherAtFour = offset(publisherInitial, publisherEpsilon, 4);
         LogicalTime publisherAtFive = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime publisherAtSix = offset(publisherInitial, publisherEpsilon, 6);
         LogicalTime frontierAtSix = offset(frontierInitial, frontierEpsilon, 6);
         LogicalTime receiverAtSix = offset(receiverInitial, receiverEpsilon, 6);

         try {
            MessageRetractionReturn below = publisher.sendInteraction(
               publisherInteraction, values, belowTag, publisherAtFour);
            JavaTckSupport.check(below != null && below.retractionHandleIsValid
                  && below.handle != null,
               "accepted timestamp-4 interaction returned no valid retraction handle");
         } catch (InvalidLogicalTime allowedBelowBoundaryRejection) {
            // The mapped assertion is the post-grant delivery cutoff; the
            // standard does not require acceptance of this below-boundary send.
         }
         MessageRetractionReturn equal = publisher.sendInteraction(
            publisherInteraction, values, equalTag, publisherAtFive);
         JavaTckSupport.check(equal != null && equal.retractionHandleIsValid && equal.handle != null,
            "accepted timestamp-5 interaction returned no valid retraction handle");
         JavaTckSupport.check(receiverRecorder.receivedTimes.isEmpty(),
            "post-grant TSO interaction was delivered before producer advancement");

         publisher.timeAdvanceRequest(publisherAtFour);
         await(callbackModel, publisher, "publisher TAR(4) grant",
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 1);
         JavaTckSupport.check(publisherAtFour.equals(
               publisherRecorder.timeAdvanceGrantTimes.get(0))
               && receiverRecorder.receivedTimes.isEmpty(),
            "late TSO was delivered while the publisher advanced to 4");

         publisher.timeAdvanceRequest(publisherAtSix);
         frontier.timeAdvanceRequest(frontierAtSix);
         await(callbackModel, "producer frontiers at 6",
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 2
               && frontierRecorder.timeAdvanceGrantTimes.size() == 2,
            publisher, frontier);
         receiver.timeAdvanceRequest(receiverAtSix);
         await(callbackModel, "receiver TAR(6) grant",
            () -> receiverRecorder.timeAdvanceGrantTimes.size() == 2,
            receiver, publisher, frontier);
         JavaTckSupport.check(receiverAtSix.equals(
               receiverRecorder.timeAdvanceGrantTimes.get(1))
               && receiverRecorder.receivedTimes.isEmpty(),
            "a timestamp at or below completed TAR(5) was delivered before TAR(6)");
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (frontierRegulating && frontierJoined) {
            try { frontier.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined && receiverInteraction != null) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (frontierJoined) resign(frontier);
         if (publisherJoined) resign(publisher);
         if (receiverJoined) resign(receiver);
         if (created) {
            try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (frontierConnected) disconnect(frontier);
         if (publisherConnected) disconnect(publisher);
         if (receiverConnected) disconnect(receiver);
      }
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static void await(CallbackModel model, String description, Condition condition,
         RTIambassador... ambassadors) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.isMet()) {
         for (RTIambassador ambassador : ambassadors) {
            if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   private static void await(CallbackModel model, RTIambassador ambassador,
         String description, Condition condition) throws Exception {
      await(model, description, condition, new RTIambassador[] {ambassador});
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK post-grant cutoff callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Recorder {
      private volatile boolean timeConstrainedEnabled;
      private volatile boolean timeRegulationEnabled;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> receivedTimes = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  receivedTimes.add((LogicalTime<?, ?>) arguments[6]);
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }
}
