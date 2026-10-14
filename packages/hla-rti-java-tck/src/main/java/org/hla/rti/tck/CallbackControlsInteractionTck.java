package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Standard receive-order interaction delivery under callback disable/enable. */
final class CallbackControlsInteractionTck {
   private CallbackControlsInteractionTck() { }

   static void run(RtiFactory factory, String fom, String interactionClassName,
         String parameterName, String objectClassName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-callback-interaction-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-callback-interaction-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-callback-interaction-receiver", federation);
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
            "callback-control interaction lookups returned inconsistent handles");
         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         byte[] payload = new byte[] {(byte) 0xc0, (byte) 0xde};
         byte[] firstTag = new byte[] {0x43, 0x42, 0x31};
         send(publisher, publisherInteraction, publisherParameter, payload, firstTag);
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.deliveries.size() == 1,
            "baseline interaction callback did not arrive");
         verify(receiverRecorder.deliveries.get(0), receiverInteraction,
            receiverParameter, payload, firstTag, producer, "baseline");

         receiverRecorder.deliveries.clear();
         receiver.disableCallbacks();
         byte[] secondTag = new byte[] {0x43, 0x42, 0x32};
         send(publisher, publisherInteraction, publisherParameter, payload, secondTag);
         serviceWhileDisabled(receiver, callbackModel, objectClassName);
         JavaTckSupport.check(receiverRecorder.deliveries.isEmpty(),
            "disabled callbacks exposed an interaction callback");

         receiver.enableCallbacks();
         await(receiver, callbackModel, objectClassName,
            () -> receiverRecorder.deliveries.size() == 1,
            "re-enabled interaction callback did not arrive");
         JavaTckSupport.check(receiverRecorder.deliveries.size() == 1,
            "re-enabled interaction callback was not delivered exactly once");
         verify(receiverRecorder.deliveries.get(0), receiverInteraction,
            receiverParameter, payload, secondTag, producer, "re-enabled");

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

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] payload, byte[] tag) throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload);
      publisher.sendInteraction(interaction, values, tag);
   }

   private static void verify(Delivery delivery, InteractionClassHandle expectedInteraction,
         ParameterHandle expectedParameter, byte[] payload, byte[] tag,
         FederateHandle producer, String phase) throws Exception {
      JavaTckSupport.check(expectedInteraction.equals(delivery.interaction)
            && delivery.parameters.size() == 1
            && Arrays.equals(payload, delivery.parameters.get(expectedParameter))
            && Arrays.equals(tag, delivery.tag)
            && producer.equals(delivery.producer)
            && delivery.transportation != null
            && (delivery.regions == null || delivery.regions.isEmpty())
            && delivery.sentOrder == OrderType.RECEIVE
            && delivery.receivedOrder == OrderType.RECEIVE,
         phase + " interaction callback lost its standard delivery metadata");
   }

   private static void serviceWhileDisabled(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      long deadline = System.nanoTime() + 100_000_000L;
      do {
         service(ambassador, model, objectClassName);
      } while (System.nanoTime() < deadline);
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

   private static final class Delivery {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;

      private Delivery(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regions = (RegionHandleSet) arguments[5];
         sentOrder = (OrderType) arguments[6];
         receivedOrder = (OrderType) arguments[7];
      }
   }

   private static final class Recorder {
      private final List<Delivery> deliveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments.length == 8) {
                  deliveries.add(new Delivery(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK interaction callback recorder";
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
