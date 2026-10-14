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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Timestamped interaction delivery using an adapter-declared custom transportation type. */
final class CustomTransportationTimestampedInteractionDeliveryTck {
   private CustomTransportationTimestampedInteractionDeliveryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String transportationName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador producer = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder producerEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-custom-tso-interaction-" + UUID.randomUUID();
      boolean producerConnected = false, receiverConnected = false, created = false;
      boolean producerJoined = false, receiverJoined = false;
      boolean published = false, subscribed = false, regulating = false, constrained = false;
      InteractionClassHandle producerInteraction = null, receiverInteraction = null;
      try {
         producer.connect(producerEvents.proxy(), callbackModel);
         producerConnected = true;
         receiver.connect(receiverEvents.proxy(), callbackModel);
         receiverConnected = true;
         producer.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producerHandle = producer.joinFederationExecution(
            "java-tck-custom-tso-interaction-producer", federation);
         producerJoined = true;
         receiver.joinFederationExecution("java-tck-custom-tso-interaction-receiver", federation);
         receiverJoined = true;

         producerInteraction = producer.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle producerParameter = producer.getParameterHandle(
            producerInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         TransportationTypeHandle producerTransportation =
            producer.getTransportationTypeHandle(transportationName);
         TransportationTypeHandle receiverTransportation =
            receiver.getTransportationTypeHandle(transportationName);
         JavaTckSupport.check(interactionClassName.equals(
               producer.getInteractionClassName(producerInteraction))
               && interactionClassName.equals(
                  receiver.getInteractionClassName(receiverInteraction))
               && parameterName.equals(producer.getParameterName(
                  producerInteraction, producerParameter))
               && parameterName.equals(receiver.getParameterName(
                  receiverInteraction, receiverParameter))
               && transportationName.equals(
                  producer.getTransportationTypeName(producerTransportation))
               && transportationName.equals(
                  receiver.getTransportationTypeName(receiverTransportation)),
            "adapter FOM interaction, parameter, or custom transport name did not round-trip");

         producer.publishInteractionClass(producerInteraction);
         published = true;
         producer.changeInteractionOrderType(producerInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = producer.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "members selected different logical-time factories");
         producer.enableTimeRegulation(epsilon);
         regulating = true;
         receiver.enableTimeConstrained();
         constrained = true;
         await(producer, receiver, callbackModel,
            () -> producerEvents.regulationEnabled && receiverEvents.constrainedEnabled,
            "time-role callbacks");

         LogicalTime initial = producer.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiver.queryLogicalTime()),
            "producer and receiver began at different logical times");
         LogicalTime messageTime = offset(initial, epsilon, 3);
         byte[] payload = new byte[] {0x00, 0x00, 0x00, 0x2a};
         byte[] tag = new byte[] {0x54, 0x53, 0x4f};
         ParameterHandleValueMap parameters =
            producer.getParameterHandleValueMapFactory().create(1);
         parameters.put(producerParameter, payload);
         MessageRetractionReturn sent = producer.sendInteraction(
            producerInteraction, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped send returned no valid retraction handle");
         JavaTckSupport.check(receiverEvents.deliveries.isEmpty(),
            "timestamped interaction arrived before the receiver's TAR");
         receiver.timeAdvanceRequest(messageTime);
         producer.timeAdvanceRequest(messageTime);
         await(producer, receiver, callbackModel,
            () -> producerEvents.grants.size() == 1 && receiverEvents.grants.size() == 1
               && receiverEvents.deliveries.size() == 1,
            "timestamped delivery and grants");

         Delivery delivery = receiverEvents.deliveries.get(0);
         JavaTckSupport.check(receiverInteraction.equals(delivery.interaction)
               && delivery.parameters.size() == 1
               && Arrays.equals(payload, delivery.parameters.get(receiverParameter))
               && Arrays.equals(tag, delivery.tag) && producerHandle.equals(delivery.producer)
               && messageTime.equals(delivery.time)
               && delivery.sentOrder == OrderType.TIMESTAMP
               && delivery.receivedOrder == OrderType.TIMESTAMP
               && sent.handle.equals(delivery.retraction),
            "timestamped callback changed payload, time, order, producer, or retraction metadata");
         JavaTckSupport.check(delivery.transportation != null
               && transportationName.equals(
                  receiver.getTransportationTypeName(delivery.transportation)),
            "timestamped callback changed custom transportation identity");
         JavaTckSupport.check(messageTime.equals(producerEvents.grants.get(0))
               && messageTime.equals(receiverEvents.grants.get(0))
               && receiverEvents.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "timestamped delivery did not precede the receiver's TAR grant");

         receiver.disableTimeConstrained(); constrained = false;
         producer.disableTimeRegulation(); regulating = false;
         receiver.unsubscribeInteractionClass(receiverInteraction); subscribed = false;
         producer.unpublishInteractionClass(producerInteraction); published = false;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && producerJoined) try { producer.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined && receiverInteraction != null) try {
            receiver.unsubscribeInteractionClass(receiverInteraction);
         } catch (Exception ignored) { }
         if (published && producerJoined && producerInteraction != null) try {
            producer.unpublishInteractionClass(producerInteraction);
         } catch (Exception ignored) { }
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (producerJoined) try { producer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { producer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (producerConnected) try { producer.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador producer, RTIambassador receiver, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(producer);
            JavaTckSupport.drain(receiver);
         }
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Delivery {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private hla.rti1516_2025.MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private boolean regulationEnabled;
      private boolean constrainedEnabled;
      private final List<LogicalTime<?, ?>> grants = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private final List<Delivery> deliveries = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) regulationEnabled = true;
               else if ("timeConstrainedEnabled".equals(name)) constrainedEnabled = true;
               else if ("receiveInteraction".equals(name) && args != null && args.length == 10) {
                  Delivery event = new Delivery();
                  event.interaction = (InteractionClassHandle) args[0];
                  event.parameters = ((ParameterHandleValueMap) args[1]).clone();
                  event.tag = ((byte[]) args[2]).clone();
                  event.transportation = (TransportationTypeHandle) args[3];
                  event.producer = (FederateHandle) args[4];
                  event.time = (LogicalTime<?, ?>) args[6];
                  event.sentOrder = (OrderType) args[7];
                  event.receivedOrder = (OrderType) args[8];
                  event.retraction = (hla.rti1516_2025.MessageRetractionHandle) args[9];
                  deliveries.add(event);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && args != null && args.length == 1) {
                  grants.add((LogicalTime<?, ?>) args[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK custom timestamped interaction callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
