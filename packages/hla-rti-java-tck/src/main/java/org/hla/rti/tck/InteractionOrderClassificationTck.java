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
import hla.rti1516_2025.RegionHandleSet;
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
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Classifies receive-order and timestamp-order interaction callbacks at one receiver. */
final class InteractionOrderClassificationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private InteractionOrderClassificationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-interaction-order-classification-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean published = false, subscribed = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-order-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-order-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherClass = publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle receiverClass = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherClass, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverClass, parameterName);
         JavaTckSupport.check(interactionName.equals(publisher.getInteractionClassName(publisherClass))
               && interactionName.equals(receiver.getInteractionClassName(receiverClass))
               && parameterName.equals(publisher.getParameterName(publisherClass, publisherParameter))
               && parameterName.equals(receiver.getParameterName(receiverClass, receiverParameter)),
            "adapter FOM did not resolve interaction and parameter handles consistently");

         publisher.publishInteractionClass(publisherClass);
         published = true;
         receiver.subscribeInteractionClass(receiverClass);
         subscribed = true;

         byte[] receiveValue = new byte[] {0x52, 0x4f};
         byte[] receiveTag = new byte[] {0x52, 0x4f};
         send(publisher, publisherClass, publisherParameter, receiveValue, receiveTag);
         await(receiver, callbackModel,
            () -> receiverCallbacks.receiveOrder.size() >= 1,
            "receive-order interaction callback");
         JavaTckSupport.check(receiverCallbacks.receiveOrder.size() == 1
               && receiverCallbacks.timestampOrder.isEmpty(),
            "ordinary Send Interaction did not use only the receive-order callback path");
         assertReceiveOrder(receiverCallbacks.receiveOrder.get(0), receiverClass,
            receiverParameter, receiveValue, receiveTag, producer, receiver);

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(receiverTimeFactory.getName()),
            "interaction-order participants selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon);
         await(publisher, callbackModel,
            () -> publisherCallbacks.timeRegulationEnabled,
            "publisher time regulation enablement");
         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.timeConstrainedEnabled,
            "receiver time constrained enablement");

         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);
         LogicalTime timestamp = offset(publisher.queryLogicalTime(), epsilon, 3);
         LogicalTime receiverTarget = offset(receiver.queryLogicalTime(), epsilon, 3);
         JavaTckSupport.check(timestamp.equals(receiverTarget),
            "publisher and receiver derived different timestamp-order advances");

         byte[] timestampValue = new byte[] {0x54, 0x53, 0x4f};
         byte[] timestampTag = new byte[] {0x54, 0x53};
         ParameterHandleValueMap timestampParameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         timestampParameters.put(publisherParameter, timestampValue.clone());
         MessageRetractionReturn retraction = publisher.sendInteraction(
            publisherClass, timestampParameters, timestampTag.clone(), timestamp);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Send Interaction returned no valid retraction handle");
         JavaTckSupport.check(receiverCallbacks.receiveOrder.size() == 1
               && receiverCallbacks.timestampOrder.isEmpty(),
            "timestamped Send Interaction was delivered before receiver time advance");

         receiverCallbacks.callbackOrder.clear();
         receiver.timeAdvanceRequest(receiverTarget);
         publisher.timeAdvanceRequest(timestamp);
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherCallbacks.timeAdvanceGrant != null
               && receiverCallbacks.timeAdvanceGrant != null
               && receiverCallbacks.timestampOrder.size() == 1,
            "timestamp-order callback and matching time-advance grants");

         JavaTckSupport.check(receiverCallbacks.receiveOrder.size() == 1
               && receiverCallbacks.timestampOrder.size() == 1,
            "same receiver did not keep receive-order and timestamp-order on distinct callback paths");
         assertTimestampOrder(receiverCallbacks.timestampOrder.get(0), receiverClass,
            receiverParameter, timestampValue, timestampTag, producer, timestamp,
            retraction.handle, receiver);
         JavaTckSupport.check(receiverCallbacks.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "timestamped interaction was not delivered before its matching grant");
         JavaTckSupport.check(timestamp.equals(publisherCallbacks.timeAdvanceGrant)
               && receiverTarget.equals(receiverCallbacks.timeAdvanceGrant)
               && timestamp.equals(publisher.queryLogicalTime())
               && receiverTarget.equals(receiver.queryLogicalTime()),
            "time-advance grants or logical-time queries returned the wrong requested times");

         receiver.unsubscribeInteractionClass(receiverClass);
         subscribed = false;
         publisher.unpublishInteractionClass(publisherClass);
         published = false;
      } finally {
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeInteractionClass(receiver.getInteractionClassHandle(interactionName)); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(publisher.getInteractionClassHandle(interactionName)); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] value, byte[] tag) throws Exception {
      ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value.clone());
      publisher.sendInteraction(interaction, parameters, tag.clone());
   }

   private static void assertReceiveOrder(Interaction event,
         InteractionClassHandle interaction, ParameterHandle parameter, byte[] value,
         byte[] tag, FederateHandle producer, RTIambassador inspector) throws Exception {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.parameters.size() == 1 && Arrays.equals(value, event.parameters.get(parameter))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty()
            && event.regionsEmpty,
         "receive-order callback returned incorrect standard metadata");
   }

   private static void assertTimestampOrder(Interaction event,
         InteractionClassHandle interaction, ParameterHandle parameter, byte[] value,
         byte[] tag, FederateHandle producer, LogicalTime<?, ?> time,
         MessageRetractionHandle retraction, RTIambassador inspector) throws Exception {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.parameters.size() == 1 && Arrays.equals(value, event.parameters.get(parameter))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && time.equals(event.time) && event.sentOrder == OrderType.TIMESTAMP
            && event.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(event.retraction) && event.regionsEmpty
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty(),
         "timestamp-order callback returned incorrect standard metadata");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            JavaTckSupport.drain(second);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Interaction(InteractionClassHandle interaction, ParameterHandleValueMap parameters,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            boolean regionsEmpty, LogicalTime<?, ?> time, OrderType sentOrder,
            OrderType receivedOrder, MessageRetractionHandle retraction) {
         this.interaction = interaction;
         this.parameters = parameters.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regionsEmpty;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
         this.retraction = retraction;
      }
   }

   private static final class Recorder {
      private final List<Interaction> receiveOrder = new CopyOnWriteArrayList<>();
      private final List<Interaction> timestampOrder = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  receiveOrder.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     arguments[5] == null || ((RegionHandleSet) arguments[5]).isEmpty(),
                     null, null, null, null));
                  callbackOrder.add(name);
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timestampOrder.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     arguments[5] == null || ((RegionHandleSet) arguments[5]).isEmpty(),
                     (LogicalTime<?, ?>) arguments[6], (OrderType) arguments[7],
                     (OrderType) arguments[8], (MessageRetractionHandle) arguments[9]));
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK interaction-order callbacks";
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
