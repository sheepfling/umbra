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
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies the strict timestamp cutoff for Time Advance Request Available. */
final class TimestampedInteractionTaraStrictTimestampCutoffTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final long NO_PROGRESS_WINDOW_NANOS = 100_000_000L;

   private TimestampedInteractionTaraStrictTimestampCutoffTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      Recorder publisherRecorder = new Recorder();
      String federation = "java-tck-tara-strict-timestamp-cutoff-" + UUID.randomUUID();
      boolean receiverConnected = false;
      boolean publisherConnected = false;
      boolean created = false;
      boolean receiverJoined = false;
      boolean publisherJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean receiverConstrained = false;
      boolean publisherRegulating = false;
      InteractionClassHandle receiverInteraction = null;
      InteractionClassHandle publisherInteraction = null;
      try {
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         receiver.joinFederationExecution("java-tck-tara-cutoff-receiver", federation);
         receiverJoined = true;
         publisher.joinFederationExecution("java-tck-tara-cutoff-publisher", federation);
         publisherJoined = true;

         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         JavaTckSupport.check(receiverInteraction.equals(publisherInteraction)
               && receiverParameter.equals(publisherParameter),
            "TARA strict timestamp cutoff handles differed between federates");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(receiverTimeFactory.getName().equals(publisherTimeFactory.getName()),
            "TARA strict timestamp cutoff federates selected different logical-time factories");
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverTimeFactory.makeEpsilon();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         JavaTckSupport.check(receiverInitial.equals(publisherInitial),
            "TARA strict timestamp cutoff federates began at different logical times");

         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(callbackModel, receiver, "receiver Time Constrained callback",
            () -> receiverRecorder.timeConstrainedEnabled);
         publisher.enableTimeRegulation(publisherEpsilon);
         publisherRegulating = true;
         await(callbackModel, publisher, "publisher Time Regulation callback",
            () -> publisherRecorder.timeRegulationEnabled);

         LogicalTime messageTime = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime taraCutoff = offset(receiverInitial, receiverEpsilon, 6);
         LogicalTime nextCutoff = offset(receiverInitial, receiverEpsilon, 7);
         JavaTckSupport.check(isBefore(messageTime, taraCutoff)
               && isBefore(taraCutoff, nextCutoff),
            "TARA strict timestamp cutoff test times were not strictly increasing");

         byte[] earlierPayload = new byte[] {0x54, 0x35};
         byte[] cutoffPayload = new byte[] {0x54, 0x36};
         byte[] earlierTag = new byte[] {'t', '5'};
         byte[] cutoffTag = new byte[] {'t', '6'};
         ParameterHandleValueMap earlierValues =
            publisher.getParameterHandleValueMapFactory().create(1);
         earlierValues.put(publisherParameter, earlierPayload);
         ParameterHandleValueMap cutoffValues =
            publisher.getParameterHandleValueMapFactory().create(1);
         cutoffValues.put(publisherParameter, cutoffPayload);
         checkRetraction(publisher.sendInteraction(
            publisherInteraction, earlierValues, earlierTag, messageTime),
            "timestamp-5 interaction");
         checkRetraction(publisher.sendInteraction(
            publisherInteraction, cutoffValues, cutoffTag, taraCutoff),
            "timestamp-6 interaction");

         publisher.timeAdvanceRequest(messageTime);
         await(callbackModel, publisher, "publisher TAR(5) grant",
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 1);
         JavaTckSupport.check(messageTime.equals(
               publisherRecorder.timeAdvanceGrantTimes.get(0))
               && receiverRecorder.interactions.isEmpty()
               && receiverRecorder.timeAdvanceGrantTimes.isEmpty(),
            "TARA strict timestamp cutoff delivered an interaction before the receiver request");

         List<String> callbackOrderBeforeFirstTara =
            new CopyOnWriteArrayList<>(receiverRecorder.callbackOrder);
         receiver.timeAdvanceRequestAvailable(taraCutoff);
         await(callbackModel, "receiver TARA(6) interaction and grant",
            () -> receiverRecorder.interactions.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.size() == 1, receiver);
         InteractionRecord first = receiverRecorder.interactions.get(0);
         JavaTckSupport.check(first.interaction.equals(receiverInteraction)
               && first.time.equals(messageTime)
               && first.sentOrder == OrderType.TIMESTAMP
               && first.receivedOrder == OrderType.TIMESTAMP,
            "TARA(6) did not deliver the timestamp-5 interaction with timestamp order");
         JavaTckSupport.check(Arrays.equals(first.parameters.get(receiverParameter), earlierPayload)
               && Arrays.equals(first.tag, earlierTag),
            "TARA(6) changed the timestamp-5 interaction parameter or tag");
         JavaTckSupport.check(taraCutoff.equals(
               receiverRecorder.timeAdvanceGrantTimes.get(0)),
            "TARA(6) returned the wrong grant time");
         checkNewCallbackOrder(receiverRecorder.callbackOrder, callbackOrderBeforeFirstTara,
            "TARA(6)");

         List<String> callbackOrderAfterFirstGrant =
            new CopyOnWriteArrayList<>(receiverRecorder.callbackOrder);
         receiver.timeAdvanceRequestAvailable(nextCutoff);
         ensureNoProgress(callbackModel, NO_PROGRESS_WINDOW_NANOS, receiver);
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.size() == 1,
            "TARA released the timestamp-6 interaction before the publisher advanced");

         publisher.timeAdvanceRequest(taraCutoff);
         await(callbackModel, "publisher TAR(6), receiver TARA(7), and timestamp-6 delivery",
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 2
               && receiverRecorder.timeAdvanceGrantTimes.size() == 2
               && receiverRecorder.interactions.size() == 2,
            publisher, receiver);
         InteractionRecord second = receiverRecorder.interactions.get(1);
         JavaTckSupport.check(second.interaction.equals(receiverInteraction)
               && second.time.equals(taraCutoff)
               && second.sentOrder == OrderType.TIMESTAMP
               && second.receivedOrder == OrderType.TIMESTAMP,
            "TARA(7) did not deliver the timestamp-6 interaction with timestamp order");
         JavaTckSupport.check(Arrays.equals(second.parameters.get(receiverParameter), cutoffPayload)
               && Arrays.equals(second.tag, cutoffTag),
            "TARA(7) changed the timestamp-6 interaction parameter or tag");
         JavaTckSupport.check(taraCutoff.equals(
               publisherRecorder.timeAdvanceGrantTimes.get(1))
               && nextCutoff.equals(receiverRecorder.timeAdvanceGrantTimes.get(1)),
            "TARA strict timestamp cutoff follow-up advances returned the wrong grant times");
         checkNewCallbackOrder(receiverRecorder.callbackOrder, callbackOrderAfterFirstGrant,
            "TARA(7)");
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined && receiverInteraction != null) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) resign(publisher);
         if (receiverJoined) resign(receiver);
         if (created) {
            try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
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
      await(model, description, condition, ambassador);
   }

   private static void ensureNoProgress(CallbackModel model, long durationNanos,
         RTIambassador ambassador) throws Exception {
      long deadline = System.nanoTime() + durationNanos;
      while (System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static boolean isBefore(LogicalTime left, LogicalTime right) throws Exception {
      return left.compareTo(right) < 0;
   }

   private static void checkRetraction(MessageRetractionReturn result, String description) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null,
         "accepted " + description + " returned no valid retraction handle");
   }

   private static void checkNewCallbackOrder(List<String> actual, List<String> before,
         String advance) {
      JavaTckSupport.check(actual.size() == before.size() + 2
            && actual.subList(0, before.size()).equals(before)
            && "interaction".equals(actual.get(before.size()))
            && "grant".equals(actual.get(before.size() + 1)),
         advance + " did not deliver the eligible interaction before its grant");
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
      if ("toString".equals(name)) return "Java TCK TARA strict timestamp cutoff callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class InteractionRecord {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;

      private InteractionRecord(InteractionClassHandle interaction,
            ParameterHandleValueMap parameters, byte[] tag, LogicalTime<?, ?> time,
            OrderType sentOrder, OrderType receivedOrder) {
         this.interaction = interaction;
         this.parameters = parameters;
         this.tag = tag.clone();
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
      }
   }

   private static final class Recorder {
      private volatile boolean timeConstrainedEnabled;
      private volatile boolean timeRegulationEnabled;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new CopyOnWriteArrayList<>();
      private final List<InteractionRecord> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

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
                  callbackOrder.add("grant");
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  interactions.add(new InteractionRecord(
                     (InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1],
                     (byte[]) arguments[2],
                     (LogicalTime<?, ?>) arguments[6],
                     (OrderType) arguments[7],
                     (OrderType) arguments[8]));
                  callbackOrder.add("interaction");
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }
}
