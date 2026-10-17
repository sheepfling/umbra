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

/** Checks receiver-dependent order metadata for one sent TSO interaction. */
final class TimestampedPerRecipientOrderConversionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedPerRecipientOrderConversionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String objectClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      RTIambassador unconstrained = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder constrainedCallbacks = new Recorder();
      Recorder unconstrainedCallbacks = new Recorder();
      String federation = "java-tck-per-recipient-order-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean constrainedConnected = false;
      boolean unconstrainedConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean constrainedJoined = false;
      boolean unconstrainedJoined = false;
      boolean published = false;
      boolean constrainedSubscribed = false;
      boolean unconstrainedSubscribed = false;
      boolean timeConstrained = false;
      boolean regulating = false;
      InteractionClassHandle senderInteraction = null;
      InteractionClassHandle constrainedInteraction = null;
      InteractionClassHandle unconstrainedInteraction = null;
      try {
         sender.connect(senderCallbacks.proxy(), callbackModel);
         senderConnected = true;
         constrained.connect(constrainedCallbacks.proxy(), callbackModel);
         constrainedConnected = true;
         unconstrained.connect(unconstrainedCallbacks.proxy(), callbackModel);
         unconstrainedConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-per-recipient-sender", federation);
         senderJoined = true;
         constrained.joinFederationExecution(
            "java-tck-per-recipient-constrained", federation);
         constrainedJoined = true;
         unconstrained.joinFederationExecution(
            "java-tck-per-recipient-unconstrained", federation);
         unconstrainedJoined = true;

         senderInteraction = sender.getInteractionClassHandle(interactionClassName);
         constrainedInteraction = constrained.getInteractionClassHandle(interactionClassName);
         unconstrainedInteraction = unconstrained.getInteractionClassHandle(interactionClassName);
         ParameterHandle senderParameter = sender.getParameterHandle(
            senderInteraction, parameterName);
         ParameterHandle constrainedParameter = constrained.getParameterHandle(
            constrainedInteraction, parameterName);
         ParameterHandle unconstrainedParameter = unconstrained.getParameterHandle(
            unconstrainedInteraction, parameterName);
         JavaTckSupport.check(senderInteraction.equals(constrainedInteraction)
               && senderInteraction.equals(unconstrainedInteraction)
               && senderParameter.equals(constrainedParameter)
               && senderParameter.equals(unconstrainedParameter),
            "adapter interaction handles did not resolve consistently across recipients");

         sender.publishInteractionClass(senderInteraction);
         published = true;
         constrained.subscribeInteractionClass(constrainedInteraction);
         constrainedSubscribed = true;
         unconstrained.subscribeInteractionClass(unconstrainedInteraction);
         unconstrainedSubscribed = true;
         sender.changeInteractionOrderType(senderInteraction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> senderFactory = sender.getTimeFactory();
         LogicalTimeFactory<?, ?> constrainedFactory = constrained.getTimeFactory();
         LogicalTimeFactory<?, ?> unconstrainedFactory = unconstrained.getTimeFactory();
         JavaTckSupport.check(senderFactory.getName().equals(constrainedFactory.getName())
               && senderFactory.getName().equals(unconstrainedFactory.getName()),
            "adapter recipients selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) senderFactory.makeEpsilon();
         LogicalTimeInterval lookahead = (LogicalTimeInterval) senderFactory.makeZero();
         for (int index = 0; index < 5; ++index) {
            lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         }

         constrained.enableTimeConstrained();
         timeConstrained = true;
         await(constrained, callbackModel, objectClassName,
            () -> constrainedCallbacks.timeConstrainedEnabled,
            "time-constrained recipient callback");
         sender.enableTimeRegulation(lookahead);
         regulating = true;
         await(sender, callbackModel, objectClassName,
            () -> senderCallbacks.timeRegulationEnabled,
            "sender time-regulation callback");

         LogicalTime initial = sender.queryLogicalTime();
         LogicalTime messageTime = offset(initial, epsilon, 6);
         LogicalTime senderTarget = offset(initial, epsilon, 2);
         byte[] payload = {0x54, 0x53, 0x4F};
         byte[] tag = {0x50, 0x52};
         ParameterHandleValueMap values = sender.getParameterHandleValueMapFactory().create(1);
         values.put(senderParameter, payload.clone());
         MessageRetractionReturn retraction = sender.sendInteraction(
            senderInteraction, values, tag.clone(), messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped TSO-preferred send returned no retraction designator");

         await(unconstrained, callbackModel, objectClassName,
            () -> unconstrainedCallbacks.timestamped.size() == 1,
            "unconstrained time-bearing RO callback");
         JavaTckSupport.check(constrainedCallbacks.timestamped.isEmpty()
               && constrainedCallbacks.timeAdvanceGrants.isEmpty(),
            "constrained recipient received TSO data before advancing to its timestamp");
         JavaTckSupport.check(unconstrainedCallbacks.receiveOrder.isEmpty()
               && unconstrainedCallbacks.timeAdvanceGrants.isEmpty()
               && unconstrainedCallbacks.sequence.equals(Arrays.asList("timestamped")),
            "unconstrained recipient did not receive exactly one time-bearing callback");

         Delivery unconstrainedDelivery = unconstrainedCallbacks.timestamped.get(0);
         checkDelivery(unconstrained, unconstrainedDelivery, unconstrainedInteraction,
            unconstrainedParameter, producer, OrderType.RECEIVE, messageTime, payload,
            tag, retraction, "unconstrained RO conversion");

         constrained.timeAdvanceRequest(messageTime);
         sender.timeAdvanceRequest(senderTarget);
         await(constrained, callbackModel, objectClassName,
            () -> constrainedCallbacks.timestamped.size() == 1
               && constrainedCallbacks.timeAdvanceGrants.size() == 1,
            "constrained TSO delivery and matching grant");
         await(sender, callbackModel, objectClassName,
            () -> senderCallbacks.timeAdvanceGrants.size() == 1,
            "sender time-advance grant");
         Delivery constrainedDelivery = constrainedCallbacks.timestamped.get(0);
         checkDelivery(constrained, constrainedDelivery, constrainedInteraction,
            constrainedParameter, producer, OrderType.TIMESTAMP, messageTime, payload,
            tag, retraction, "constrained TSO delivery");
         JavaTckSupport.check(messageTime.equals(constrainedCallbacks.timeAdvanceGrants.get(0))
               && messageTime.equals(constrained.queryLogicalTime())
               && constrainedCallbacks.sequence.equals(
                  Arrays.asList("timestamped", "timeAdvanceGrant")),
            "constrained TSO callback did not precede its matching grant");
         JavaTckSupport.check(unconstrainedCallbacks.timestamped.size() == 1
               && unconstrainedCallbacks.sequence.equals(Arrays.asList("timestamped")),
            "unconstrained recipient received a duplicate message or grant");
      } finally {
         if (timeConstrained && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && senderJoined) {
            try { sender.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (constrainedSubscribed && constrainedJoined) {
            try { constrained.unsubscribeInteractionClass(constrainedInteraction); }
            catch (Exception ignored) { }
         }
         if (unconstrainedSubscribed && unconstrainedJoined) {
            try { unconstrained.unsubscribeInteractionClass(unconstrainedInteraction); }
            catch (Exception ignored) { }
         }
         if (published && senderJoined) {
            try { sender.unpublishInteractionClass(senderInteraction); }
            catch (Exception ignored) { }
         }
         if (unconstrainedJoined) {
            try { unconstrained.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (constrainedJoined) {
            try { constrained.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (senderJoined) {
            try { sender.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { sender.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (unconstrainedConnected) {
            try { unconstrained.disconnect(); } catch (Exception ignored) { }
         }
         if (constrainedConnected) {
            try { constrained.disconnect(); } catch (Exception ignored) { }
         }
         if (senderConnected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void checkDelivery(RTIambassador receiver, Delivery delivery,
         InteractionClassHandle interaction, ParameterHandle parameter,
         FederateHandle producer, OrderType receivedOrder, LogicalTime messageTime,
         byte[] payload, byte[] tag, MessageRetractionReturn sentRetraction,
         String description) throws Exception {
      TransportationTypeHandle transportation = delivery.transportation;
      JavaTckSupport.check(interaction.equals(delivery.interaction)
            && delivery.parameters.size() == 1
            && Arrays.equals(payload, delivery.parameters.get(parameter))
            && Arrays.equals(tag, delivery.tag)
            && producer.equals(delivery.producer)
            && messageTime.equals(delivery.time)
            && delivery.sentOrder == OrderType.TIMESTAMP
            && delivery.receivedOrder == receivedOrder
            && delivery.retractionPresent
            && sentRetraction.handle.equals(delivery.retraction)
            && transportation != null
            && !receiver.getTransportationTypeName(transportation).isEmpty(),
         description + " did not preserve the sent message's standard data");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 0.0);
         } else {
            ambassador.getObjectClassHandle(objectClassName);
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Delivery {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final LogicalTime time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final boolean retractionPresent;
      private final Object retraction;

      private Delivery(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retractionPresent = arguments[9] != null;
         retraction = arguments[9];
      }
   }

   private static final class Recorder {
      private final List<Boolean> receiveOrder = new CopyOnWriteArrayList<>();
      private final List<Delivery> timestamped = new CopyOnWriteArrayList<>();
      private final List<LogicalTime> timeAdvanceGrants = new CopyOnWriteArrayList<>();
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
                  boolean isTimestamped = arguments != null && arguments.length >= 10
                     && arguments[6] instanceof LogicalTime;
                  if (isTimestamped) {
                     timestamped.add(new Delivery(arguments));
                     sequence.add("timestamped");
                  } else {
                     receiveOrder.add(Boolean.TRUE);
                     sequence.add("receiveOrder");
                  }
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrants.add((LogicalTime) arguments[0]);
                  sequence.add("timeAdvanceGrant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK per-recipient order callbacks";
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
