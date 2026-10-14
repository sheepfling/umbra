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

/** Callback gating for the standard available time-advance request services. */
final class CallbackControlsAvailableTimeAdvanceTck {
   private CallbackControlsAvailableTimeAdvanceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String objectClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-callback-available-advance-" + UUID.randomUUID();
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
            "java-tck-available-advance-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-available-advance-receiver", federation);
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
            "available-advance FOM lookups returned inconsistent interaction handles");
         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(receiverTimeFactory.getName()),
            "available-advance federates selected different logical-time factories");
         LogicalTimeInterval epsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);
         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.timeConstrainedEnabled,
            "timeConstrainedEnabled callback did not arrive");
         publisher.enableTimeRegulation(lookahead);
         publisherRegulating = true;
         await(publisher, callbackModel, objectClassName,
            () -> publisherRecorder.timeRegulationEnabled,
            "timeRegulationEnabled callback did not arrive");
         publisherRecorder.clearEvents();
         receiverRecorder.clearEvents();

         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiverInitial),
            "available-advance federates began at different logical times");

         runAvailableAdvance(publisher, receiver, publisherRecorder, receiverRecorder,
            publisherInteraction, receiverInteraction, publisherParameter, receiverParameter,
            producer, callbackModel, objectClassName, initial, receiverInitial, epsilon,
            false, 7, 7, 2,
            new byte[] {0x41, 0x56, 0x41, 0x2d, 0x54, 0x41, 0x52},
            new byte[] {0x41, 0x56, 0x41, 0x2d, 0x31},
            "timeAdvanceRequestAvailable");

         publisherRecorder.clearEvents();
         receiverRecorder.clearEvents();
         runAvailableAdvance(publisher, receiver, publisherRecorder, receiverRecorder,
            publisherInteraction, receiverInteraction, publisherParameter, receiverParameter,
            producer, callbackModel, objectClassName, initial, receiverInitial, epsilon,
            true, 9, 10, 4,
            new byte[] {0x41, 0x56, 0x41, 0x2d, 0x4e, 0x4d, 0x52},
            new byte[] {0x41, 0x56, 0x41, 0x2d, 0x32},
            "nextMessageRequestAvailable");

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
               receiver.unsubscribeInteractionClass(
                  receiver.getInteractionClassHandle(interactionClassName));
            } catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try {
               publisher.unpublishInteractionClass(
                  publisher.getInteractionClassHandle(interactionClassName));
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

   private static void runAvailableAdvance(RTIambassador publisher, RTIambassador receiver,
         Recorder publisherRecorder, Recorder receiverRecorder,
         InteractionClassHandle publisherInteraction, InteractionClassHandle receiverInteraction,
         ParameterHandle publisherParameter, ParameterHandle receiverParameter,
         FederateHandle producer, CallbackModel callbackModel, String objectClassName,
         LogicalTime<?, ?> publisherInitial, LogicalTime<?, ?> receiverInitial,
         LogicalTimeInterval epsilon, boolean nextMessage, int messageSteps,
         int requestSteps, int publisherSteps, byte[] payload, byte[] tag,
         String description) throws Exception {
      LogicalTime<?, ?> messageTime = offset(publisherInitial, epsilon, messageSteps);
      LogicalTime<?, ?> requestTime = offset(receiverInitial, epsilon, requestSteps);
      LogicalTime<?, ?> publisherTarget = offset(publisherInitial, epsilon, publisherSteps);
      ParameterHandleValueMap parameters =
         publisher.getParameterHandleValueMapFactory().create(1);
      parameters.put(publisherParameter, payload);
      MessageRetractionReturn retraction = publisher.sendInteraction(
         publisherInteraction, parameters, tag, messageTime);
      JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
            && retraction.handle != null,
         description + " send returned an invalid retraction handle");

      receiver.disableCallbacks();
      if (nextMessage) receiver.nextMessageRequestAvailable(requestTime);
      else receiver.timeAdvanceRequestAvailable(requestTime);
      JavaTckSupport.check(receiverRecorder.timedInteractions.isEmpty()
            && receiverRecorder.timeAdvanceGrantTimes.isEmpty(),
         description + " completed before the regulating federate advanced");
      publisher.timeAdvanceRequest(publisherTarget);
      await(publisher, callbackModel, objectClassName,
         () -> publisherRecorder.timeAdvanceGrantTimes.size() == 1,
         description + " publisher timeAdvanceGrant did not arrive");
      JavaTckSupport.check(publisherTarget.equals(publisherRecorder.timeAdvanceGrantTimes.get(0)),
         description + " publisher received the wrong grant time");
      service(receiver, callbackModel, objectClassName);
      JavaTckSupport.check(receiverRecorder.timedInteractions.isEmpty()
            && receiverRecorder.timeAdvanceGrantTimes.isEmpty(),
         description + " exposed callbacks while callbacks were disabled");

      receiver.enableCallbacks();
      await(receiver, callbackModel, objectClassName,
         () -> receiverRecorder.timedInteractions.size() == 1
            && receiverRecorder.timeAdvanceGrantTimes.size() == 1,
         description + " interaction and timeAdvanceGrant did not arrive after enabling");
      JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
            Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
         description + " delivered its grant before the timestamped interaction");
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
         description + " returned incorrect timestamped interaction metadata");
      JavaTckSupport.check(receiverRecorder.flushQueueGrantTimes.isEmpty(),
         description + " returned an unexpected Flush Queue grant");
      JavaTckSupport.check(messageTime.equals(receiverRecorder.timeAdvanceGrantTimes.get(0))
            && messageTime.equals(receiver.queryLogicalTime()),
         description + " returned the wrong time-advance grant");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(ambassador, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private interface Condition {
      boolean isTrue();
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) {
         result = (LogicalTimeInterval) result.add(epsilon);
      }
      return result;
   }

   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime<?, ?> result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime<?, ?>) ((LogicalTime) result).add(interval);
      }
      return result;
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
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new ArrayList<>();
      private final List<TimedInteraction> timedInteractions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();

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
                  callbackOrder.add(name);
               } else if ("receiveInteraction".equals(name) && arguments.length == 10) {
                  timedInteractions.add(new TimedInteraction(arguments));
                  callbackOrder.add(name);
               } else if ("flushQueueGrant".equals(name) && arguments.length == 2) {
                  flushQueueGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK available-advance callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearEvents() {
         timeAdvanceGrantTimes.clear();
         flushQueueGrantTimes.clear();
         timedInteractions.clear();
         callbackOrder.clear();
      }
   }
}
