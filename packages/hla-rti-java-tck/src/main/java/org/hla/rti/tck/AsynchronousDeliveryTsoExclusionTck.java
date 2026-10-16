package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Ensures asynchronous delivery releases RO traffic but not a constrained TSO. */
final class AsynchronousDeliveryTsoExclusionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private AsynchronousDeliveryTsoExclusionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String timestampedClassName, String timestampedParameterName,
         String receiveOrderClassName, String receiveOrderParameterName,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-async-tso-gate-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean receiverJoined = false;
      boolean senderPublishedTimestamped = false;
      boolean senderPublishedReceiveOrder = false;
      boolean receiverSubscribedTimestamped = false;
      boolean receiverSubscribedReceiveOrder = false;
      boolean timeConstrained = false;
      boolean asynchronous = false;
      InteractionClassHandle senderTimestampedClass = null;
      InteractionClassHandle receiverTimestampedClass = null;
      InteractionClassHandle senderReceiveOrderClass = null;
      InteractionClassHandle receiverReceiveOrderClass = null;
      try {
         sender.connect(senderCallbacks.proxy(), callbackModel);
         senderConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-async-tso-sender", federation);
         senderJoined = true;
         receiver.joinFederationExecution("java-tck-async-tso-receiver", federation);
         receiverJoined = true;

         senderTimestampedClass = sender.getInteractionClassHandle(timestampedClassName);
         receiverTimestampedClass = receiver.getInteractionClassHandle(timestampedClassName);
         senderReceiveOrderClass = sender.getInteractionClassHandle(receiveOrderClassName);
         receiverReceiveOrderClass = receiver.getInteractionClassHandle(receiveOrderClassName);
         ParameterHandle senderTimestampedParameter = sender.getParameterHandle(
            senderTimestampedClass, timestampedParameterName);
         ParameterHandle receiverTimestampedParameter = receiver.getParameterHandle(
            receiverTimestampedClass, timestampedParameterName);
         ParameterHandle senderReceiveOrderParameter = sender.getParameterHandle(
            senderReceiveOrderClass, receiveOrderParameterName);
         ParameterHandle receiverReceiveOrderParameter = receiver.getParameterHandle(
            receiverReceiveOrderClass, receiveOrderParameterName);
         JavaTckSupport.check(senderTimestampedClass.equals(receiverTimestampedClass)
               && senderReceiveOrderClass.equals(receiverReceiveOrderClass)
               && senderTimestampedParameter.equals(receiverTimestampedParameter)
               && senderReceiveOrderParameter.equals(receiverReceiveOrderParameter),
            "adapter interaction handles did not resolve consistently across members");

         sender.publishInteractionClass(senderTimestampedClass);
         senderPublishedTimestamped = true;
         sender.publishInteractionClass(senderReceiveOrderClass);
         senderPublishedReceiveOrder = true;
         sender.changeInteractionOrderType(senderTimestampedClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverTimestampedClass);
         receiverSubscribedTimestamped = true;
         receiver.subscribeInteractionClass(receiverReceiveOrderClass);
         receiverSubscribedReceiveOrder = true;

         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "adapter members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime initial = sender.queryLogicalTime();
         LogicalTime tsoTimeFive = offset(initial, epsilon, 5);
         LogicalTime tsoTimeSix = offset(initial, epsilon, 6);
         LogicalTime receiverTargetFive = offset(receiver.queryLogicalTime(), epsilon, 5);
         LogicalTime receiverTargetSix = offset(receiver.queryLogicalTime(), epsilon, 6);
         LogicalTimeInterval lookahead = (LogicalTimeInterval) timeFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         timeConstrained = true;
         await(receiver, callbackModel, objectClassName,
            () -> receiverCallbacks.timeConstrainedEnabled,
            "time-constrained role callback");
         sender.enableTimeRegulation(lookahead);
         await(sender, callbackModel, objectClassName,
            () -> senderCallbacks.timeRegulationEnabled,
            "time-regulation role callback");
         receiver.enableAsynchronousDelivery();
         asynchronous = true;

         byte[] roValue = {0x52, 0x4f};
         byte[] roTag = {0x52, 0x4f, 0x31};
         ParameterHandleValueMap receiveOrderParameters =
            sender.getParameterHandleValueMapFactory().create(1);
         receiveOrderParameters.put(senderReceiveOrderParameter, roValue.clone());
         byte[] tsoTagFiveFirst = {0x54, 0x35, 0x31};
         byte[] tsoTagFiveSecond = {0x54, 0x35, 0x32};
         byte[] tsoTagSix = {0x54, 0x36, 0x31};
         sendTimestamped(sender, senderTimestampedClass, senderTimestampedParameter,
            (byte) 0x71, tsoTagFiveFirst, tsoTimeFive);
         sendTimestamped(sender, senderTimestampedClass, senderTimestampedParameter,
            (byte) 0x72, tsoTagFiveSecond, tsoTimeFive);
         sendTimestamped(sender, senderTimestampedClass, senderTimestampedParameter,
            (byte) 0x73, tsoTagSix, tsoTimeSix);
         sender.sendInteraction(senderReceiveOrderClass, receiveOrderParameters, roTag.clone());
         sender.timeAdvanceRequest(tsoTimeSix);

         await(receiver, callbackModel, objectClassName,
            () -> !receiverCallbacks.receiveOrder.isEmpty(),
            "receive-order progress while asynchronous delivery is enabled");
         JavaTckSupport.check(receiverCallbacks.timestamped.isEmpty()
               && receiverCallbacks.timeAdvanceGrants.isEmpty(),
            "asynchronous delivery bypassed the constrained receiver's TSO time gate");
         Delivery receiveOrder = receiverCallbacks.receiveOrder.get(0);
         JavaTckSupport.check(receiverCallbacks.receiveOrder.size() == 1
               && receiverReceiveOrderClass.equals(receiveOrder.interaction)
               && receiveOrder.parameters.size() == 1
               && Arrays.equals(roValue,
                  receiveOrder.parameters.get(receiverReceiveOrderParameter))
               && Arrays.equals(roTag, receiveOrder.tag)
               && producer.equals(receiveOrder.producer),
            "asynchronous receive-order progress returned the wrong application data");

         receiver.timeAdvanceRequest(receiverTargetFive);
         await(receiver, callbackModel, objectClassName,
            () -> receiverCallbacks.timestamped.size() == 2
               && receiverCallbacks.timeAdvanceGrants.size() == 1,
            "equal-time timestamped interactions release before the TAR(5) grant");
         byte[][] expectedValues = {{(byte) 0x71}, {(byte) 0x72}};
         byte[][] expectedTags = {tsoTagFiveFirst, tsoTagFiveSecond};
         for (int index = 0; index < 2; ++index) {
            Delivery timestamped = receiverCallbacks.timestamped.get(index);
            JavaTckSupport.check(receiverTimestampedClass.equals(timestamped.interaction)
                  && timestamped.parameters.size() == 1
                  && Arrays.equals(expectedValues[index],
                     timestamped.parameters.get(receiverTimestampedParameter))
                  && Arrays.equals(expectedTags[index], timestamped.tag)
                  && producer.equals(timestamped.producer)
                  && tsoTimeFive.equals(timestamped.time)
                  && timestamped.sentOrder == OrderType.TIMESTAMP
                  && timestamped.receivedOrder == OrderType.TIMESTAMP,
               "equal-time timestamped interaction returned incorrect metadata");
         }
         JavaTckSupport.check(receiverTargetFive.equals(
               receiverCallbacks.timeAdvanceGrants.get(0)),
            "TAR(5) returned the wrong grant time");
         for (int pass = 0; pass < 8; ++pass) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               receiver.evokeMultipleCallbacks(0.0, 0.0);
            } else {
               receiver.getObjectClassHandle(objectClassName);
               Thread.yield();
            }
         }
         JavaTckSupport.check(receiverCallbacks.timestamped.size() == 2
               && receiverCallbacks.timeAdvanceGrants.size() == 1,
            "callback servicing after TAR(5) delivered another TSO event");
         receiver.timeAdvanceRequest(receiverTargetSix);
         await(receiver, callbackModel, objectClassName,
            () -> receiverCallbacks.timestamped.size() == 3
               && receiverCallbacks.timeAdvanceGrants.size() == 2,
            "timestamp-6 interaction release before the TAR(6) grant");
         Delivery timestampSix = receiverCallbacks.timestamped.get(2);
         JavaTckSupport.check(receiverTimestampedClass.equals(timestampSix.interaction)
               && timestampSix.parameters.size() == 1
               && Arrays.equals(new byte[] {(byte) 0x73},
                  timestampSix.parameters.get(receiverTimestampedParameter))
               && Arrays.equals(tsoTagSix, timestampSix.tag)
               && producer.equals(timestampSix.producer)
               && tsoTimeSix.equals(timestampSix.time)
               && receiverTargetSix.equals(receiverCallbacks.timeAdvanceGrants.get(1)),
            "future TSO interaction was not held until TAR(6)");
         JavaTckSupport.check(receiverCallbacks.sequence.equals(
               Arrays.asList("receiveOrder", "timestamped", "timestamped",
                  "timeAdvanceGrant", "timestamped", "timeAdvanceGrant")),
            "timestamped callback and TAR grant order was incorrect");
      } finally {
         if (asynchronous && receiverJoined) {
            try { receiver.disableAsynchronousDelivery(); } catch (Exception ignored) { }
         }
         if (timeConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (receiverSubscribedReceiveOrder && receiverJoined) {
            try { receiver.unsubscribeInteractionClass(receiverReceiveOrderClass); }
            catch (Exception ignored) { }
         }
         if (receiverSubscribedTimestamped && receiverJoined) {
            try { receiver.unsubscribeInteractionClass(receiverTimestampedClass); }
            catch (Exception ignored) { }
         }
         if (senderPublishedReceiveOrder && senderJoined) {
            try { sender.unpublishInteractionClass(senderReceiveOrderClass); }
            catch (Exception ignored) { }
         }
         if (senderPublishedTimestamped && senderJoined) {
            try { sender.unpublishInteractionClass(senderTimestampedClass); }
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
            try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (senderConnected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   /** Verifies that ordinary Send Message preserves a sender already Time Granted. */
   static void runSendWhileTimeGranted(RtiFactory factory, String fom,
         String timeImplementation, String interactionClassName,
         String parameterName, String objectClassName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-send-message-time-granted-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean timeConstrained = false;
      boolean asynchronous = false;
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
            "java-tck-send-state-sender", federation);
         senderJoined = true;
         receiver.joinFederationExecution("java-tck-send-state-receiver", federation);
         receiverJoined = true;

         senderInteraction = sender.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle senderParameter = sender.getParameterHandle(
            senderInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(senderInteraction.equals(receiverInteraction)
               && senderParameter.equals(receiverParameter),
            "adapter interaction handles did not resolve consistently");

         sender.publishInteractionClass(senderInteraction);
         published = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;
         receiver.enableTimeConstrained();
         timeConstrained = true;
         await(receiver, callbackModel, objectClassName,
            () -> receiverCallbacks.timeConstrainedEnabled,
            "time-constrained role callback");
         receiver.enableAsynchronousDelivery();
         asynchronous = true;

         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(
               receiver.getTimeFactory().getName()),
            "adapter members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = (LogicalTimeInterval) timeFactory.makeEpsilon();
         sender.enableTimeRegulation(lookahead);
         await(sender, callbackModel, objectClassName,
            () -> senderCallbacks.timeRegulationEnabled,
            "time-regulation role callback");

         LogicalTime initial = sender.queryLogicalTime();
         LogicalTime firstTarget = offset(initial, epsilon, 10);
         LogicalTime secondTarget = offset(initial, epsilon, 20);
         sender.timeAdvanceRequest(firstTarget);
         await(sender, callbackModel, objectClassName,
            () -> senderCallbacks.timeAdvanceGrants.size() == 1,
            "initial Time Advance Grant");
         JavaTckSupport.check(firstTarget.equals(senderCallbacks.timeAdvanceGrants.get(0))
               && firstTarget.equals(sender.queryLogicalTime()),
            "sender did not enter Time Granted at the requested logical time");

         byte[] value = {0x41, 0x42};
         byte[] tag = {0x54, 0x47};
         ParameterHandleValueMap values =
            sender.getParameterHandleValueMapFactory().create(1);
         values.put(senderParameter, value.clone());
         sender.sendInteraction(senderInteraction, values, tag.clone());
         await(receiver, callbackModel, objectClassName,
            () -> !receiverCallbacks.receiveOrder.isEmpty(),
            "receive-order Send Message while sender is Time Granted");

         Delivery received = receiverCallbacks.receiveOrder.get(0);
         JavaTckSupport.check(receiverCallbacks.receiveOrder.size() == 1
               && receiverCallbacks.timestamped.isEmpty()
               && receiverCallbacks.timeAdvanceGrants.isEmpty()
               && receiverCallbacks.sequence.equals(Arrays.asList("receiveOrder"))
               && receiverInteraction.equals(received.interaction)
               && received.parameters.size() == 1
               && Arrays.equals(value, received.parameters.get(receiverParameter))
               && Arrays.equals(tag, received.tag)
               && producer.equals(received.producer),
            "Time Granted Send Message delivered unexpected data or callbacks");
         JavaTckSupport.check(senderCallbacks.timeAdvanceGrants.size() == 1
               && firstTarget.equals(sender.queryLogicalTime()),
            "Send Message displaced the sender's existing grant or logical time");

         sender.timeAdvanceRequest(secondTarget);
         await(sender, callbackModel, objectClassName,
            () -> senderCallbacks.timeAdvanceGrants.size() == 2,
            "subsequent Time Advance Grant");
         JavaTckSupport.check(secondTarget.equals(senderCallbacks.timeAdvanceGrants.get(1))
               && secondTarget.equals(sender.queryLogicalTime()),
            "sender could not advance after Send Message while Time Granted");
      } finally {
         if (asynchronous && receiverJoined) {
            try { receiver.disableAsynchronousDelivery(); } catch (Exception ignored) { }
         }
         if (timeConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
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

   private static void sendTimestamped(RTIambassador sender,
         InteractionClassHandle interactionClass, ParameterHandle parameter,
         byte value, byte[] tag, LogicalTime time) throws Exception {
      ParameterHandleValueMap values = sender.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, new byte[] {value});
      Object result = sender.sendInteraction(interactionClass, values, tag.clone(), time);
      JavaTckSupport.check(result != null,
         "timestamped interaction send returned no result");
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
      private final FederateHandle producer;
      private final LogicalTime time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;

      private Delivery(Object[] arguments, boolean timestamped) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = timestamped ? (LogicalTime) arguments[6] : null;
         sentOrder = timestamped ? (OrderType) arguments[7] : OrderType.RECEIVE;
         receivedOrder = timestamped ? (OrderType) arguments[8] : OrderType.RECEIVE;
      }
   }

   private static final class Recorder {
      private final List<Delivery> receiveOrder = new CopyOnWriteArrayList<>();
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
                  Delivery delivery = new Delivery(arguments, isTimestamped);
                  if (isTimestamped) {
                     timestamped.add(delivery);
                     sequence.add("timestamped");
                  } else {
                     receiveOrder.add(delivery);
                     sequence.add("receiveOrder");
                  }
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrants.add((LogicalTime) arguments[0]);
                  sequence.add("timeAdvanceGrant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK asynchronous TSO gate callbacks";
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
