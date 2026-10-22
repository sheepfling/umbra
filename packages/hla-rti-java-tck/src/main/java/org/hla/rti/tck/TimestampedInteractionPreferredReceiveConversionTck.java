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

/** Checks timestamped-send conversion for a class whose preferred order is RECEIVE. */
final class TimestampedInteractionPreferredReceiveConversionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedInteractionPreferredReceiveConversionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String objectClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-preferred-receive-conversion-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean constrained = false;
      boolean asynchronous = false;
      boolean regulating = false;
      InteractionClassHandle senderInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      try {
         sender.connect(senderCallbacks.proxy(), callbackModel);
         senderConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-preferred-receive-sender", federation);
         senderJoined = true;
         receiver.joinFederationExecution("java-tck-preferred-receive-receiver", federation);
         receiverJoined = true;

         senderInteraction = sender.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle senderParameter = sender.getParameterHandle(
            senderInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(senderInteraction.equals(receiverInteraction)
               && senderParameter.equals(receiverParameter),
            "adapter interaction handles did not resolve consistently across members");

         sender.publishInteractionClass(senderInteraction);
         published = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;
         sender.changeInteractionOrderType(senderInteraction, OrderType.RECEIVE);

         LogicalTimeFactory<?, ?> senderFactory = sender.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(senderFactory.getName().equals(receiverFactory.getName()),
            "adapter members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) senderFactory.makeEpsilon();
         LogicalTimeInterval lookahead = (LogicalTimeInterval) senderFactory.makeZero();
         for (int index = 0; index < 5; ++index) {
            lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         }

         receiver.enableTimeConstrained();
         constrained = true;
         await(receiver, callbackModel, objectClassName,
            () -> receiverCallbacks.timeConstrainedEnabled,
            "time-constrained role callback");
         receiver.enableAsynchronousDelivery();
         asynchronous = true;
         sender.enableTimeRegulation(lookahead);
         regulating = true;
         await(sender, callbackModel, objectClassName,
            () -> senderCallbacks.timeRegulationEnabled,
            "time-regulation role callback");

         LogicalTime initial = sender.queryLogicalTime();
         LogicalTime messageTime = offset(initial, epsilon, 6);
         LogicalTime senderTarget = offset(initial, epsilon, 2);
         byte[] value = {0x52, 0x4F};
         byte[] tag = {0x50, 0x52};
         ParameterHandleValueMap values =
            sender.getParameterHandleValueMapFactory().create(1);
         values.put(senderParameter, value.clone());
         MessageRetractionReturn retraction = sender.sendInteraction(
            senderInteraction, values, tag.clone(), messageTime);
         JavaTckSupport.check(retraction != null && !retraction.retractionHandleIsValid
               && retraction.handle == null,
            "timestamped send with preferred RECEIVE returned a retraction designator"
               + " (valid=" + (retraction == null ? "null" : retraction.retractionHandleIsValid)
               + ", handle=" + (retraction == null || retraction.handle == null ? "null" : "present") + ")");

         await(receiver, callbackModel, objectClassName,
            () -> receiverCallbacks.timestamped.size() == 1,
            "preferred-RECEIVE timestamped callback");
         JavaTckSupport.check(receiverCallbacks.receiveOrder.isEmpty()
               && receiverCallbacks.timeAdvanceGrants.isEmpty(),
            "preferred-RECEIVE conversion used an untimed callback or granted early");
         Delivery received = receiverCallbacks.timestamped.get(0);
         TransportationTypeHandle transportation = received.transportation;
         JavaTckSupport.check(receiverCallbacks.timestamped.size() == 1
               && receiverCallbacks.sequence.equals(Arrays.asList("timestamped"))
               && receiverInteraction.equals(received.interaction)
               && received.parameters.size() == 1
               && Arrays.equals(value, received.parameters.get(receiverParameter))
               && Arrays.equals(tag, received.tag)
               && producer.equals(received.producer)
               && messageTime.equals(received.time)
               && received.sentOrder == OrderType.RECEIVE
               && received.receivedOrder == OrderType.RECEIVE
               && !received.retractionPresent
               && transportation != null
               && !receiver.getTransportationTypeName(transportation).isEmpty(),
            "preferred-RECEIVE callback returned incorrect data, order, time, or retraction metadata");

         receiver.timeAdvanceRequest(messageTime);
         sender.timeAdvanceRequest(senderTarget);
         await(receiver, callbackModel, objectClassName,
            () -> receiverCallbacks.timeAdvanceGrants.size() == 1,
            "preferred-RECEIVE matching receiver grant");
         await(sender, callbackModel, objectClassName,
            () -> senderCallbacks.timeAdvanceGrants.size() == 1,
            "preferred-RECEIVE sender grant");
         JavaTckSupport.check(messageTime.equals(receiverCallbacks.timeAdvanceGrants.get(0))
               && messageTime.equals(receiver.queryLogicalTime())
               && receiverCallbacks.sequence.equals(
                  Arrays.asList("timestamped", "timeAdvanceGrant")),
            "preferred-RECEIVE callback did not precede the matching grant");
      } finally {
         if (asynchronous && receiverJoined) {
            try { receiver.disableAsynchronousDelivery(); } catch (Exception ignored) { }
         }
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && senderJoined) {
            try { sender.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (published && senderJoined) {
            try { sender.unpublishInteractionClass(senderInteraction); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (senderConnected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
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
                  boolean isTimestamped = arguments != null && arguments.length >= 10;
                  if (isTimestamped) {
                     timestamped.add(new Delivery(arguments));
                     sequence.add("timestamped");
                  } else {
                     receiveOrder.add(Boolean.TRUE);
                  }
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrants.add((LogicalTime) arguments[0]);
                  sequence.add("timeAdvanceGrant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK preferred-RECEIVE callbacks";
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
