package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Verifies ordinary send success, publication-loss rejection, and republish recovery. */
final class InteractionPublicationSendTransitionTck {
   private InteractionPublicationSendTransitionTck() { }

   static void run(RtiFactory factory, String fom, String interactionName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-interaction-publication-transition-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean published = false, subscribed = false;
      InteractionClassHandle publisherInteraction = null, receiverInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-publication-transition-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-publication-transition-receiver", federation);
         receiverJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         byte[] value = new byte[] {0x50, 0x54};
         byte[] tag = new byte[] {0x54, 0x31};
         send(publisher, publisherInteraction, publisherParameter, value, tag);
         awaitCount(receiver, receiverRecorder, 1,
            "published interaction did not arrive before unpublication");
         assertInteraction(receiverRecorder.interactions.get(0), receiverInteraction,
            receiverParameter, value, tag, producer, receiver,
            "delivery before unpublication");

         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         InteractionClassHandle unpublishedInteraction = publisherInteraction;
         JavaTckSupport.expectFailure(
            () -> send(publisher, unpublishedInteraction, publisherParameter, value, tag),
            "InteractionClassNotPublished", "sending after unpublication");
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1,
            "rejected unpublished send produced a receiver callback");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         send(publisher, publisherInteraction, publisherParameter, value, tag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(receiverRecorder.interactions.size() == 1,
               "republished interaction arrived before callback servicing in evoked mode");
         }
         awaitCount(receiver, receiverRecorder, 2,
            "republishing did not restore ordinary interaction delivery");
         JavaTckSupport.check(receiverRecorder.interactions.size() == 2,
            "publication transition produced an unexpected callback count");
         assertInteraction(receiverRecorder.interactions.get(1), receiverInteraction,
            receiverParameter, value, tag, producer, receiver,
            "delivery after republication");
         JavaTckSupport.check(publisherRecorder.interactions.isEmpty(),
            "republished interaction looped a callback back to its publisher");

         receiver.unsubscribeInteractionClass(receiverInteraction);
         subscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
      } finally {
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
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
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] value, byte[] tag) throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value);
      publisher.sendInteraction(interaction, values, tag);
   }

   private static void awaitCount(RTIambassador ambassador, Recorder recorder, int count,
         String message) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && recorder.interactions.size() < count) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(recorder.interactions.size() >= count, message);
   }

   private static void assertInteraction(Interaction event,
         InteractionClassHandle interaction, ParameterHandle parameter, byte[] value,
         byte[] tag, FederateHandle producer, RTIambassador inspector, String description)
         throws Exception {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.parameters.size() == 1 && event.parameters.containsKey(parameter)
            && Arrays.equals(value, event.parameters.get(parameter))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && event.transportation != null
            && "HLAreliable".equals(inspector.getTransportationTypeName(event.transportation))
            && event.regionsEmpty,
         description + " returned invalid class, parameter, payload, tag, producer, "
            + "transportation, or region metadata");
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  event.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK interaction publication transition";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
