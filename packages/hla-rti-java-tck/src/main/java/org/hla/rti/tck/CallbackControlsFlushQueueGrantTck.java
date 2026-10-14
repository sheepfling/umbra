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
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Portable callback gating for Flush Queue grants and queued TSO interaction delivery. */
final class CallbackControlsFlushQueueGrantTck {
   private CallbackControlsFlushQueueGrantTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String objectClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-callback-flush-queue-grant-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean publisherRegulating = false;
      boolean receiverConstrained = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-flush-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-flush-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            "Flush Queue FOM lookups returned inconsistent interaction handles");
         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(receiverTimeFactory.getName()),
            "Flush Queue federation members selected different logical-time factories");
         LogicalTimeInterval epsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.timeConstrainedEnabled,
            "timeConstrainedEnabled callback did not arrive");
         publisher.enableTimeRegulation(epsilon);
         publisherRegulating = true;
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.timeRegulationEnabled,
            "timeRegulationEnabled callback did not arrive");
         publisherRecorder.clearTimeAndInteractionEvents();
         receiverRecorder.clearTimeAndInteractionEvents();

         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiverInitial),
            "Flush Queue federates began at different logical times");
         LogicalTime<?, ?> messageTime = offset(initial, epsilon, 9);
         LogicalTime<?, ?> publisherTarget = offset(initial, epsilon, 4);
         LogicalTime<?, ?> requestBoundary = offset(receiverInitial, epsilon, 10);
         byte[] payload = new byte[] {0x46, 0x51, 0x2d, 0x43, 0x54, 0x52, 0x4c};
         byte[] tag = new byte[] {0x46, 0x51, 0x2d, 0x54, 0x41, 0x47};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         MessageRetractionReturn retraction = publisher.sendInteraction(
            publisherInteraction, parameters, tag, messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Flush Queue interaction returned no valid retraction handle");

         receiver.disableCallbacks();
         receiver.flushQueueRequest(requestBoundary);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(receiverRecorder.timedInteractions.isEmpty()
                  && receiverRecorder.flushQueueGrantTimes.isEmpty(),
               "Flush Queue callbacks arrived before evoked callback servicing");
         }
         publisher.timeAdvanceRequest(publisherTarget);
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 1,
            "publisher timeAdvanceGrant did not arrive");
         JavaTckSupport.check(publisherTarget.equals(
               publisherRecorder.timeAdvanceGrantTimes.get(0)),
            "publisher time advance granted the wrong logical time");

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            receiver.evokeMultipleCallbacks(0.0, 0.0);
         } else {
            service(receiver, objectClassName);
         }
         JavaTckSupport.check(receiverRecorder.timedInteractions.isEmpty()
               && receiverRecorder.flushQueueGrantTimes.isEmpty(),
            "disabled callbacks exposed a Flush Queue grant or queued interaction");

         receiver.enableCallbacks();
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.timedInteractions.size() == 1
               && receiverRecorder.flushQueueGrantTimes.size() == 1,
            "re-enabled Flush Queue grant and queued interaction did not both arrive");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "flushQueueGrant")),
            "Flush Queue grant arrived before queued timestamped interaction");

         TimedInteraction interaction = receiverRecorder.timedInteractions.get(0);
         JavaTckSupport.check(receiverInteraction.equals(interaction.interaction)
               && interaction.parameters.size() == 1
               && Arrays.equals(payload, interaction.parameters.get(receiverParameter))
               && Arrays.equals(tag, interaction.tag)
               && producer.equals(interaction.producer)
               && messageTime.equals(interaction.time)
               && interaction.sentOrder == OrderType.TIMESTAMP
               && interaction.receivedOrder == OrderType.TIMESTAMP
               && interaction.transportation != null
               && (interaction.regions == null || interaction.regions.isEmpty())
               && retraction.handle.equals(interaction.retraction),
            "Flush Queue interaction callback lost standard delivery metadata");

         LogicalTime<?, ?> grantTime = receiverRecorder.flushQueueGrantTimes.get(0);
         LogicalTime<?, ?> optimisticTime = receiverRecorder.flushQueueOptimisticTime;
         JavaTckSupport.check(grantTime != null && optimisticTime != null
               && messageTime.equals(optimisticTime)
               && compareTime(grantTime, receiverInitial) >= 0
               && compareTime(grantTime, requestBoundary) <= 0
               && grantTime.equals(receiver.queryLogicalTime()),
            "Flush Queue grant returned incomplete or out-of-bounds logical-time metadata");

         receiver.disableTimeConstrained();
         receiverConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
         receiver.unsubscribeInteractionClass(receiverInteraction);
         subscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try {
               InteractionClassHandle interaction =
                  receiver.getInteractionClassHandle(interactionClassName);
               receiver.unsubscribeInteractionClass(interaction);
            } catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try {
               InteractionClassHandle interaction =
                  publisher.getInteractionClassHandle(interactionClassName);
               publisher.unpublishInteractionClass(interaction);
            } catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); }
            catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         if (model == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 1.0);
         } else {
            service(ambassador, objectClassName);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, String objectClassName)
         throws Exception {
      ambassador.getObjectClassHandle(objectClassName);
   }

   private interface Condition {
      boolean isTrue();
   }

   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime<?, ?> result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime<?, ?>) ((LogicalTime) result).add(interval);
      }
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compareTime(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   private static final class TimedInteraction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private TimedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regions = (RegionHandleSet) arguments[5];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<TimedInteraction> timedInteractions = new ArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private LogicalTime<?, ?> flushQueueOptimisticTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name) && arguments.length == 10) {
                  timedInteractions.add(new TimedInteraction(arguments));
                  callbackOrder.add(name);
               } else if ("flushQueueGrant".equals(name) && arguments.length == 2) {
                  flushQueueGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  flushQueueOptimisticTime = (LogicalTime<?, ?>) arguments[1];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK Flush Queue callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private void clearTimeAndInteractionEvents() {
         timeAdvanceGrantTimes.clear();
         timedInteractions.clear();
         flushQueueGrantTimes.clear();
         callbackOrder.clear();
         flushQueueOptimisticTime = null;
      }
   }
}
