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

/** Checks the FOM's preferred interaction order defaults without changing either order. */
final class InteractionFddPreferredOrderDefaultTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private InteractionFddPreferredOrderDefaultTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String timestampPreferredClassName, String receivePreferredClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-interaction-fdd-order-default-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean receiverJoined = false;
      boolean regulating = false;
      boolean timestampPublished = false;
      boolean receivePublished = false;
      boolean timestampSubscribed = false;
      boolean receiveSubscribed = false;
      InteractionClassHandle senderTimestampClass = null;
      InteractionClassHandle senderReceiveClass = null;
      InteractionClassHandle receiverTimestampClass = null;
      InteractionClassHandle receiverReceiveClass = null;
      try {
         sender.connect(senderCallbacks.proxy(), callbackModel);
         senderConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-interaction-fdd-order-sender", federation);
         senderJoined = true;
         receiver.joinFederationExecution("java-tck-interaction-fdd-order-receiver", federation);
         receiverJoined = true;

         senderTimestampClass = sender.getInteractionClassHandle(timestampPreferredClassName);
         senderReceiveClass = sender.getInteractionClassHandle(receivePreferredClassName);
         receiverTimestampClass = receiver.getInteractionClassHandle(timestampPreferredClassName);
         receiverReceiveClass = receiver.getInteractionClassHandle(receivePreferredClassName);
         ParameterHandle senderTimestampParameter = sender.getParameterHandle(
            senderTimestampClass, parameterName);
         ParameterHandle senderReceiveParameter = sender.getParameterHandle(
            senderReceiveClass, parameterName);
         ParameterHandle receiverTimestampParameter = receiver.getParameterHandle(
            receiverTimestampClass, parameterName);
         ParameterHandle receiverReceiveParameter = receiver.getParameterHandle(
            receiverReceiveClass, parameterName);
         JavaTckSupport.check(senderTimestampClass != null && senderReceiveClass != null
               && receiverTimestampClass != null && receiverReceiveClass != null
               && senderTimestampParameter != null && senderReceiveParameter != null
               && receiverTimestampParameter != null && receiverReceiveParameter != null,
            "adapter FOM did not provide both preferred-order interaction classes and parameters");

         sender.publishInteractionClass(senderTimestampClass);
         timestampPublished = true;
         sender.publishInteractionClass(senderReceiveClass);
         receivePublished = true;
         receiver.subscribeInteractionClass(receiverTimestampClass);
         timestampSubscribed = true;
         receiver.subscribeInteractionClass(receiverReceiveClass);
         receiveSubscribed = true;

         LogicalTimeFactory<?, ?> senderFactory = sender.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(senderFactory.getName().equals(receiverFactory.getName()),
            "FDD-default interaction members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) senderFactory.makeEpsilon();
         LogicalTime<?, ?> initial = sender.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiverInitial),
            "FDD-default interaction members began at different logical times");
         LogicalTime<?, ?> timestampTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> receiveTime = offset(initial, epsilon, 6);
         LogicalTimeInterval lookahead = (LogicalTimeInterval) senderFactory.makeEpsilon();

         sender.enableTimeRegulation(lookahead);
         regulating = true;
         await(sender, callbackModel, timestampPreferredClassName,
            () -> senderCallbacks.timeRegulationEnabled,
            "sender time-regulation callback");

         byte[] payload = {0x4f, 0x4b};
         byte[] tag = {0x46, 0x44, 0x44};
         ParameterHandleValueMap timestampParameters =
            sender.getParameterHandleValueMapFactory().create(1);
         timestampParameters.put(senderTimestampParameter, payload.clone());
         MessageRetractionReturn timestampRetraction = sender.sendInteraction(
            senderTimestampClass, timestampParameters, tag.clone(), timestampTime);
         ParameterHandleValueMap receiveParameters =
            sender.getParameterHandleValueMapFactory().create(1);
         receiveParameters.put(senderReceiveParameter, payload.clone());
         MessageRetractionReturn receiveRetraction = sender.sendInteraction(
            senderReceiveClass, receiveParameters, tag.clone(), receiveTime);
         JavaTckSupport.check(timestampRetraction != null
               && timestampRetraction.retractionHandleIsValid
               && timestampRetraction.handle != null,
            "FDD-preferred timestamp-order send did not return a retraction designator");
         JavaTckSupport.check(receiveRetraction != null
               && !receiveRetraction.retractionHandleIsValid
               && receiveRetraction.handle == null,
            "FDD-preferred receive-order send unexpectedly returned a retraction designator");

         await(receiver, callbackModel, timestampPreferredClassName,
            () -> receiverCallbacks.timestamped.size() == 2,
            "both FDD-preferred timestamp-bearing interaction callbacks");
         Delivery timestampDelivery = receiverCallbacks.forClass(receiverTimestampClass);
         Delivery receiveDelivery = receiverCallbacks.forClass(receiverReceiveClass);
         JavaTckSupport.check(receiverCallbacks.timestamped.size() == 2
               && receiverCallbacks.receiveOrder.isEmpty()
               && timestampDelivery != null && receiveDelivery != null
               && timestampDelivery.parameters.size() == 1
               && Arrays.equals(payload,
                  timestampDelivery.parameters.get(receiverTimestampParameter))
               && Arrays.equals(tag, timestampDelivery.tag)
               && producer.equals(timestampDelivery.producer)
               && timestampTime.equals(timestampDelivery.time)
               && timestampDelivery.sentOrder == OrderType.TIMESTAMP
               && timestampDelivery.retraction != null
               && timestampRetraction.handle.equals(timestampDelivery.retraction)
               && receiveDelivery.parameters.size() == 1
               && Arrays.equals(payload,
                  receiveDelivery.parameters.get(receiverReceiveParameter))
               && Arrays.equals(tag, receiveDelivery.tag)
               && producer.equals(receiveDelivery.producer)
               && receiveTime.equals(receiveDelivery.time)
               && receiveDelivery.sentOrder == OrderType.RECEIVE
               && receiveDelivery.retraction == null,
            "timestamped interaction callbacks did not follow the FOM preferred-order defaults");
         JavaTckSupport.check(receiverCallbacks.sequence.size() == 2
               && receiverCallbacks.sequence.stream().allMatch("timestamped"::equals),
            "FDD-default sends did not produce exactly two time-bearing callbacks");
         TransportationTypeHandle timestampTransportation = timestampDelivery.transportation;
         TransportationTypeHandle receiveTransportation = receiveDelivery.transportation;
         JavaTckSupport.check(timestampTransportation != null && receiveTransportation != null
               && !receiver.getTransportationTypeName(timestampTransportation).isEmpty()
               && !receiver.getTransportationTypeName(receiveTransportation).isEmpty(),
            "FDD-default callbacks returned an unresolvable transportation type");
      } finally {
         if (regulating && senderJoined) {
            try { sender.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (receiveSubscribed && receiverJoined && receiverReceiveClass != null) {
            try { receiver.unsubscribeInteractionClass(receiverReceiveClass); }
            catch (Exception ignored) { }
         }
         if (timestampSubscribed && receiverJoined && receiverTimestampClass != null) {
            try { receiver.unsubscribeInteractionClass(receiverTimestampClass); }
            catch (Exception ignored) { }
         }
         if (receivePublished && senderJoined && senderReceiveClass != null) {
            try { sender.unpublishInteractionClass(senderReceiveClass); }
            catch (Exception ignored) { }
         }
         if (timestampPublished && senderJoined && senderTimestampClass != null) {
            try { sender.unpublishInteractionClass(senderTimestampClass); }
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
         if (model == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 0.0);
         } else {
            ambassador.getInteractionClassHandle(interactionName);
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
      private final Object retraction;

      private Delivery(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime) arguments[6];
         sentOrder = (OrderType) arguments[7];
         retraction = arguments[9];
      }
   }

   private static final class Recorder {
      private final List<Boolean> receiveOrder = new CopyOnWriteArrayList<>();
      private final List<Delivery> timestamped = new CopyOnWriteArrayList<>();
      private final List<String> sequence = new CopyOnWriteArrayList<>();
      private volatile boolean timeRegulationEnabled;

      private Delivery forClass(InteractionClassHandle handle) {
         return timestamped.stream().filter(item -> item.interaction.equals(handle))
            .findFirst().orElse(null);
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("receiveInteraction".equals(name)) {
                  if (arguments != null && arguments.length >= 10) {
                     timestamped.add(new Delivery(arguments));
                     sequence.add("timestamped");
                  } else {
                     receiveOrder.add(Boolean.TRUE);
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK FDD preferred-order callbacks";
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
