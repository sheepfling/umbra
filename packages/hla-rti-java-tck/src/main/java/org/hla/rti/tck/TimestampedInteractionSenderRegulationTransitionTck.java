package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks §8.1.4 interaction order classification across a sender-role transition. */
final class TimestampedInteractionSenderRegulationTransitionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedInteractionSenderRegulationTransitionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      Recorder regulatorCallbacks = new Recorder();
      String federation = "java-tck-interaction-sender-role-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean receiverConnected = false;
      boolean regulatorConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean receiverJoined = false;
      boolean regulatorJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean constrained = false;
      boolean asynchronous = false;
      boolean senderRegulating = false;
      boolean regulatorRegulating = false;
      InteractionClassHandle senderInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      try {
         sender.connect(senderCallbacks.proxy(), callbackModel);
         senderConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         regulatorConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-interaction-sender-role-sender", federation);
         senderJoined = true;
         receiver.joinFederationExecution(
            "java-tck-interaction-sender-role-receiver", federation);
         receiverJoined = true;
         regulator.joinFederationExecution(
            "java-tck-interaction-sender-role-regulator", federation);
         regulatorJoined = true;

         senderInteraction = sender.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle senderParameter = sender.getParameterHandle(
            senderInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(senderInteraction != null && receiverInteraction != null
               && senderParameter != null && receiverParameter != null,
            "adapter FOM did not resolve interaction and parameter handles");

         sender.publishInteractionClass(senderInteraction);
         published = true;
         sender.changeInteractionOrderType(senderInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> senderFactory = sender.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         JavaTckSupport.check(senderFactory.getName().equals(receiverFactory.getName())
               && senderFactory.getName().equals(regulatorFactory.getName()),
            "sender-role interaction members selected different logical-time factories");
         LogicalTimeInterval senderEpsilon = (LogicalTimeInterval) senderFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorFactory.makeEpsilon();
         LogicalTime<?, ?> senderInitial = sender.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         JavaTckSupport.check(senderInitial.equals(receiverInitial)
               && senderInitial.equals(regulatorInitial),
            "sender-role interaction members began at different logical times");
         LogicalTimeInterval senderLookahead = (LogicalTimeInterval) senderFactory.makeZero();
         LogicalTimeInterval regulatorLookahead =
            (LogicalTimeInterval) regulatorFactory.makeZero();
         for (int step = 0; step < 5; ++step) {
            senderLookahead = (LogicalTimeInterval) senderLookahead.add(senderEpsilon);
            regulatorLookahead =
               (LogicalTimeInterval) regulatorLookahead.add(regulatorEpsilon);
         }
         LogicalTime<?, ?> messageTime = offset(senderInitial, senderEpsilon, 6);
         LogicalTime<?, ?> receiverTarget = offset(receiverInitial, receiverEpsilon, 6);
         LogicalTime<?, ?> senderTarget = offset(senderInitial, senderEpsilon, 2);
         LogicalTime<?, ?> regulatorTarget = offset(regulatorInitial, regulatorEpsilon, 1);

         receiver.enableTimeConstrained();
         constrained = true;
         await(receiver, callbackModel, interactionName,
            () -> receiverCallbacks.timeConstrainedEnabled,
            "receiver time-constrained callback");
         receiver.enableAsynchronousDelivery();
         asynchronous = true;
         regulator.enableTimeRegulation(regulatorLookahead);
         regulatorRegulating = true;
         await(regulator, callbackModel, interactionName,
            () -> regulatorCallbacks.timeRegulationEnabled,
            "separate GALT-regulator callback");

         byte[] firstPayload = {0x41, 0x42};
         byte[] firstTag = {0x31};
         ParameterHandleValueMap firstParameters =
            sender.getParameterHandleValueMapFactory().create(1);
         firstParameters.put(senderParameter, firstPayload.clone());
         MessageRetractionReturn firstRetraction = sender.sendInteraction(
            senderInteraction, firstParameters, firstTag.clone(), messageTime);
         JavaTckSupport.check(firstRetraction != null
               && !firstRetraction.retractionHandleIsValid
               && firstRetraction.handle == null,
            "pre-regulation timestamped send returned a retraction handle");
         await(receiver, callbackModel, interactionName,
            () -> receiverCallbacks.timestamped.size() == 1,
            "pre-regulation receive-order interaction");
         Delivery first = receiverCallbacks.timestamped.get(0);
         JavaTckSupport.check(receiverCallbacks.receiveOrder.isEmpty()
               && receiverCallbacks.timeAdvanceGrants.isEmpty()
               && receiverCallbacks.sequence.equals(Arrays.asList("timestamped"))
               && receiverInteraction.equals(first.interaction)
               && first.parameters.size() == 1
               && Arrays.equals(firstPayload, first.parameters.get(receiverParameter))
               && Arrays.equals(firstTag, first.tag)
               && producer.equals(first.producer)
               && messageTime.equals(first.time)
               && first.sentOrder == OrderType.RECEIVE
               && first.receivedOrder == OrderType.RECEIVE
               && first.retraction == null
               && first.transportation != null
               && !receiver.getTransportationTypeName(first.transportation).isEmpty(),
            "unregulated sender's timestamped interaction did not arrive in receive order");

         // Keep preferred order and timestamp fixed; only sender role changes.
         sender.enableTimeRegulation(senderLookahead);
         senderRegulating = true;
         await(sender, callbackModel, interactionName,
            () -> senderCallbacks.timeRegulationEnabled,
            "sender time-regulation callback");
         byte[] secondPayload = {0x54, 0x53, 0x4f};
         byte[] secondTag = {0x32};
         ParameterHandleValueMap secondParameters =
            sender.getParameterHandleValueMapFactory().create(1);
         secondParameters.put(senderParameter, secondPayload.clone());
         MessageRetractionReturn secondRetraction = sender.sendInteraction(
            senderInteraction, secondParameters, secondTag.clone(), messageTime);
         JavaTckSupport.check(secondRetraction != null
               && secondRetraction.retractionHandleIsValid
               && secondRetraction.handle != null,
            "post-regulation timestamped send did not return a retraction handle");
         JavaTckSupport.check(receiverCallbacks.timestamped.size() == 1
               && receiverCallbacks.timeAdvanceGrants.isEmpty(),
            "post-regulation TSO interaction arrived before receiver advancement");

         receiver.timeAdvanceRequest(receiverTarget);
         sender.timeAdvanceRequest(senderTarget);
         regulator.timeAdvanceRequest(regulatorTarget);
         awaitBoth(sender, receiver, regulator, callbackModel, interactionName,
            () -> receiverCallbacks.timestamped.size() == 2
                  && receiverCallbacks.timeAdvanceGrants.size() == 1,
            "post-regulation timestamp-order interaction and receiver grant");
         Delivery second = receiverCallbacks.timestamped.get(1);
         JavaTckSupport.check(receiverCallbacks.receiveOrder.isEmpty()
               && receiverCallbacks.sequence.equals(Arrays.asList(
                  "timestamped", "timestamped", "timeAdvanceGrant"))
               && receiverInteraction.equals(second.interaction)
               && second.parameters.size() == 1
               && Arrays.equals(secondPayload, second.parameters.get(receiverParameter))
               && Arrays.equals(secondTag, second.tag)
               && producer.equals(second.producer)
               && messageTime.equals(second.time)
               && second.sentOrder == OrderType.TIMESTAMP
               && second.receivedOrder == OrderType.TIMESTAMP
               && secondRetraction.handle.equals(second.retraction)
               && receiverTarget.equals(receiverCallbacks.timeAdvanceGrants.get(0))
               && second.transportation != null
               && !receiver.getTransportationTypeName(second.transportation).isEmpty(),
            "regulated sender's timestamped interaction did not arrive as TSO before its grant");
      } finally {
         if (asynchronous && receiverJoined) {
            try { receiver.disableAsynchronousDelivery(); } catch (Exception ignored) { }
         }
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (senderRegulating && senderJoined) {
            try { sender.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (regulatorRegulating && regulatorJoined) {
            try { regulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined && receiverInteraction != null) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (published && senderJoined && senderInteraction != null) {
            try { sender.unpublishInteractionClass(senderInteraction); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (regulatorJoined) {
            try { regulator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (senderJoined) {
            try { sender.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { sender.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (regulatorConnected) {
            try { regulator.disconnect(); } catch (Exception ignored) { }
         }
         if (senderConnected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String interactionName, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         pump(ambassador, model, interactionName);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador sender, RTIambassador receiver,
         RTIambassador regulator, CallbackModel model, String interactionName,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         pump(sender, model, interactionName);
         pump(receiver, model, interactionName);
         pump(regulator, model, interactionName);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model,
         String interactionName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 0.0);
      } else {
         ambassador.getInteractionClassHandle(interactionName);
         Thread.yield();
      }
   }

   private static final class Delivery {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final Object retraction;

      private Delivery(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = arguments[9];
      }
   }

   private static final class Recorder {
      private final List<Boolean> receiveOrder = new CopyOnWriteArrayList<>();
      private final List<Delivery> timestamped = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new CopyOnWriteArrayList<>();
      private final List<String> sequence = new CopyOnWriteArrayList<>();
      private volatile boolean timeConstrainedEnabled;
      private volatile boolean timeRegulationEnabled;

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
               } else if ("receiveInteraction".equals(name)) {
                  if (arguments != null && arguments.length >= 10) {
                     timestamped.add(new Delivery(arguments));
                     sequence.add("timestamped");
                  } else {
                     receiveOrder.add(Boolean.TRUE);
                  }
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
                  sequence.add("timeAdvanceGrant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK sender-regulation interaction callbacks";
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
