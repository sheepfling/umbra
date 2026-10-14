package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
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

/** Timestamped directed delivery retaining an adapter-declared custom FOM transport. */
final class CustomTransportationTimestampedDirectedDeliveryTck {
   private CustomTransportationTimestampedDirectedDeliveryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, String transportationName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador producer = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder producerEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-custom-tso-directed-" + UUID.randomUUID();
      boolean producerConnected = false, receiverConnected = false, created = false;
      boolean producerJoined = false, receiverJoined = false;
      boolean publishedAttributes = false, subscribedAttributes = false;
      boolean publishedDirected = false, subscribedDirected = false;
      boolean regulating = false, constrained = false;
      ObjectClassHandle producerClass = null, receiverClass = null;
      InteractionClassHandle producerInteraction = null, receiverInteraction = null;
      AttributeHandle producerAttribute = null, receiverAttribute = null;
      try {
         producer.connect(producerEvents.proxy(), callbackModel);
         producerConnected = true;
         receiver.connect(receiverEvents.proxy(), callbackModel);
         receiverConnected = true;
         producer.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producerHandle = producer.joinFederationExecution(
            "java-tck-custom-tso-directed-producer", federation);
         producerJoined = true;
         receiver.joinFederationExecution("java-tck-custom-tso-directed-receiver", federation);
         receiverJoined = true;

         producerClass = producer.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         producerAttribute = producer.getAttributeHandle(producerClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
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
         JavaTckSupport.check(objectClassName.equals(producer.getObjectClassName(producerClass))
               && objectClassName.equals(receiver.getObjectClassName(receiverClass))
               && attributeName.equals(producer.getAttributeName(producerClass, producerAttribute))
               && attributeName.equals(receiver.getAttributeName(receiverClass, receiverAttribute))
               && interactionClassName.equals(
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
            "adapter FOM lookup names did not round-trip");

         AttributeHandleSet producerAttributes = JavaTckSupport.attributeSet(
            producer, producerAttribute);
         AttributeHandleSet receiverAttributes = JavaTckSupport.attributeSet(
            receiver, receiverAttribute);
         producer.publishObjectClassAttributes(producerClass, producerAttributes);
         publishedAttributes = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribedAttributes = true;
         producer.publishObjectClassDirectedInteractions(producerClass,
            JavaTckSupport.interactionSet(producer, producerInteraction));
         publishedDirected = true;
         producer.changeInteractionOrderType(producerInteraction, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         subscribedDirected = true;

         ObjectInstanceHandle target = producer.registerObjectInstance(producerClass);
         await(producer, receiver, callbackModel,
            () -> receiverEvents.discovered.equals(target), "directed target discovery");

         LogicalTimeFactory<?, ?> timeFactory = producer.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "producer and receiver selected different logical-time factories");
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
         byte[] tag = new byte[] {0x54, 0x53, 0x4f, 0x2d, 0x44};
         ParameterHandleValueMap parameters =
            producer.getParameterHandleValueMapFactory().create(1);
         parameters.put(producerParameter, payload);
         MessageRetractionReturn sent = producer.sendDirectedInteraction(
            producerInteraction, target, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped directed send returned no valid retraction handle");
         JavaTckSupport.check(receiverEvents.deliveries.isEmpty(),
            "timestamped directed interaction arrived before the receiver's TAR");
         receiver.timeAdvanceRequest(messageTime);
         producer.timeAdvanceRequest(messageTime);
         await(producer, receiver, callbackModel,
            () -> producerEvents.grants.size() == 1 && receiverEvents.grants.size() == 1
               && receiverEvents.deliveries.size() == 1,
            "timestamped directed delivery and grants");

         Delivery delivery = receiverEvents.deliveries.get(0);
         JavaTckSupport.check(delivery.interaction.equals(receiverInteraction)
               && delivery.target.equals(target) && delivery.parameters.size() == 1
               && Arrays.equals(payload, delivery.parameters.get(receiverParameter))
               && Arrays.equals(tag, delivery.tag) && producerHandle.equals(delivery.producer)
               && messageTime.equals(delivery.time)
               && delivery.sentOrder == OrderType.TIMESTAMP
               && delivery.receivedOrder == OrderType.TIMESTAMP
               && sent.handle.equals(delivery.retraction),
            "timestamped directed callback changed target, payload, time, order, or retraction");
         JavaTckSupport.check(delivery.transportation != null
               && transportationName.equals(
                  receiver.getTransportationTypeName(delivery.transportation)),
            "timestamped directed callback changed custom transportation identity");
         JavaTckSupport.check(messageTime.equals(producerEvents.grants.get(0))
               && messageTime.equals(receiverEvents.grants.get(0))
               && receiverEvents.callbackOrder.equals(
                  Arrays.asList("receiveDirectedInteraction", "timeAdvanceGrant")),
            "timestamped directed delivery or time-advance grant order was incorrect");

         receiver.disableTimeConstrained(); constrained = false;
         producer.disableTimeRegulation(); regulating = false;
         receiver.unsubscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         subscribedDirected = false;
         receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribedAttributes = false;
         producer.unpublishObjectClassDirectedInteractions(producerClass,
            JavaTckSupport.interactionSet(producer, producerInteraction));
         publishedDirected = false;
         producer.unpublishObjectClassAttributes(producerClass, producerAttributes);
         publishedAttributes = false;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && producerJoined) try { producer.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribedDirected && receiverJoined && receiverClass != null
               && receiverInteraction != null) try {
            receiver.unsubscribeObjectClassDirectedInteractions(receiverClass,
               JavaTckSupport.interactionSet(receiver, receiverInteraction));
         } catch (Exception ignored) { }
         if (subscribedAttributes && receiverJoined && receiverClass != null
               && receiverAttribute != null) try {
            receiver.unsubscribeObjectClassAttributes(receiverClass,
               JavaTckSupport.attributeSet(receiver, receiverAttribute));
         } catch (Exception ignored) { }
         if (publishedDirected && producerJoined && producerClass != null
               && producerInteraction != null) try {
            producer.unpublishObjectClassDirectedInteractions(producerClass,
               JavaTckSupport.interactionSet(producer, producerInteraction));
         } catch (Exception ignored) { }
         if (publishedAttributes && producerJoined && producerClass != null
               && producerAttribute != null) try {
            producer.unpublishObjectClassAttributes(producerClass,
               JavaTckSupport.attributeSet(producer, producerAttribute));
         } catch (Exception ignored) { }
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (producerJoined) try { producer.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
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
      private ObjectInstanceHandle target;
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
      private ObjectInstanceHandle discovered;
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
               if ("discoverObjectInstance".equals(name) && args != null && args.length > 0) {
                  discovered = (ObjectInstanceHandle) args[0];
               } else if ("timeRegulationEnabled".equals(name)) {
                  regulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  constrainedEnabled = true;
               } else if ("receiveDirectedInteraction".equals(name)
                     && args != null && args.length == 10) {
                  Delivery event = new Delivery();
                  event.interaction = (InteractionClassHandle) args[0];
                  event.target = (ObjectInstanceHandle) args[1];
                  event.parameters = ((ParameterHandleValueMap) args[2]).clone();
                  event.tag = ((byte[]) args[3]).clone();
                  event.transportation = (TransportationTypeHandle) args[4];
                  event.producer = (FederateHandle) args[5];
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
                  if ("toString".equals(name)) return "Java TCK custom timestamped directed callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
