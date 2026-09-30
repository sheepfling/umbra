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

/** Verifies ordinary receive-order FIFO fan-out to two active subscribers. */
final class InteractionMultiRecipientFifoTck {
   private InteractionMultiRecipientFifoTck() { }

   static void run(RtiFactory factory, String fom, String interactionName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      String federation = "java-tck-interaction-multi-recipient-fifo-" + UUID.randomUUID();
      boolean publisherConnected = false, firstConnected = false, secondConnected = false;
      boolean created = false, publisherJoined = false, firstJoined = false, secondJoined = false;
      boolean published = false, firstSubscribed = false, secondSubscribed = false;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle firstInteraction = null;
      InteractionClassHandle secondInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         first.connect(firstRecorder.proxy(), callbackModel);
         firstConnected = true;
         second.connect(secondRecorder.proxy(), callbackModel);
         secondConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-multi-recipient-publisher", federation);
         publisherJoined = true;
         first.joinFederationExecution("java-tck-multi-recipient-first", federation);
         firstJoined = true;
         second.joinFederationExecution("java-tck-multi-recipient-second", federation);
         secondJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         firstInteraction = first.getInteractionClassHandle(interactionName);
         secondInteraction = second.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle firstParameter =
            first.getParameterHandle(firstInteraction, parameterName);
         ParameterHandle secondParameter =
            second.getParameterHandle(secondInteraction, parameterName);

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         first.subscribeInteractionClass(firstInteraction);
         firstSubscribed = true;
         second.subscribeInteractionClass(secondInteraction);
         secondSubscribed = true;

         byte[] firstValue = new byte[] {0x46, 0x49, 0x52, 0x53, 0x54};
         byte[] firstTag = new byte[] {0x31};
         byte[] secondValue = new byte[] {0x53, 0x45, 0x43, 0x4f, 0x4e, 0x44};
         byte[] secondTag = new byte[] {0x32};
         send(publisher, publisherInteraction, publisherParameter, firstValue, firstTag);
         send(publisher, publisherInteraction, publisherParameter, secondValue, secondTag);

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(firstRecorder.interactions.isEmpty()
                  && secondRecorder.interactions.isEmpty(),
               "evoked multi-recipient interactions arrived before callback servicing");
         }
         awaitBoth(first, second, firstRecorder, secondRecorder);
         JavaTckSupport.check(firstRecorder.interactions.size() == 2
               && secondRecorder.interactions.size() == 2,
            "each active subscriber must receive exactly two interactions");
         assertDelivery(firstRecorder.interactions, firstInteraction, firstParameter,
            firstValue, firstTag, secondValue, secondTag, producer, first,
            "first subscriber");
         assertDelivery(secondRecorder.interactions, secondInteraction, secondParameter,
            firstValue, firstTag, secondValue, secondTag, producer, second,
            "second subscriber");
         JavaTckSupport.check(publisherRecorder.interactions.isEmpty(),
            "ordinary interactions were delivered back to the publisher");

         first.unsubscribeInteractionClass(firstInteraction);
         firstSubscribed = false;
         second.unsubscribeInteractionClass(secondInteraction);
         secondSubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
      } finally {
         if (secondSubscribed && secondJoined) {
            try { second.unsubscribeInteractionClass(secondInteraction); }
            catch (Exception ignored) { }
         }
         if (firstSubscribed && firstJoined) {
            try { first.unsubscribeInteractionClass(firstInteraction); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (secondJoined) {
            try { second.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (firstJoined) {
            try { first.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (secondConnected) {
            try { second.disconnect(); } catch (Exception ignored) { }
         }
         if (firstConnected) {
            try { first.disconnect(); } catch (Exception ignored) { }
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

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         Recorder firstRecorder, Recorder secondRecorder) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (firstRecorder.interactions.size() < 2 || secondRecorder.interactions.size() < 2)) {
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         Thread.yield();
      }
      JavaTckSupport.check(firstRecorder.interactions.size() >= 2
            && secondRecorder.interactions.size() >= 2,
         "both multi-recipient interaction streams did not arrive before deadline");
   }

   private static void assertDelivery(List<Interaction> received,
         InteractionClassHandle interaction, ParameterHandle parameter,
         byte[] firstValue, byte[] firstTag, byte[] secondValue, byte[] secondTag,
         FederateHandle producer, RTIambassador inspector, String description) throws Exception {
      assertRecord(received.get(0), interaction, parameter, firstValue, firstTag,
         producer, inspector, description + " first receive-order callback");
      assertRecord(received.get(1), interaction, parameter, secondValue, secondTag,
         producer, inspector, description + " second receive-order callback");
   }

   private static void assertRecord(Interaction event, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] value, byte[] tag, FederateHandle producer,
         RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.parameters.size() == 1 && event.parameters.containsKey(parameter)
            && Arrays.equals(value, event.parameters.get(parameter))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty()
            && event.regionsEmpty,
         description + " returned incorrect interaction, parameter, value, tag, producer, "
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
                  if ("toString".equals(name)) return "Java TCK multi-recipient interaction FIFO";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
