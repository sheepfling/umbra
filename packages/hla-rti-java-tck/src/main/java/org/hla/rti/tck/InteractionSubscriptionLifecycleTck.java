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

/** Verifies passive, active, downgraded, and withdrawn interaction subscriptions. */
final class InteractionSubscriptionLifecycleTck {
   private InteractionSubscriptionLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String interactionName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-interaction-subscription-lifecycle-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false;
      boolean subscribed = false, published = false;
      InteractionClassHandle publisherInteraction = null, subscriberInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-subscription-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-subscription-receiver", federation);
         subscriberJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         subscriberInteraction = subscriber.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle subscriberParameter =
            subscriber.getParameterHandle(subscriberInteraction, parameterName);
         publisher.publishInteractionClass(publisherInteraction);
         published = true;

         subscriber.subscribeInteractionClassPassively(subscriberInteraction);
         subscribed = true;
         byte[] passiveValue = new byte[] {0x50, 0x41, 0x53};
         byte[] passiveTag = new byte[] {0x50, 0x30};
         send(publisher, publisherInteraction, publisherParameter, passiveValue, passiveTag);
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberRecorder.interactions.isEmpty(),
            "passive interaction subscription unexpectedly received an interaction");

         subscriber.subscribeInteractionClass(subscriberInteraction);
         JavaTckSupport.check(subscriberRecorder.interactions.isEmpty(),
            "activating a passive interaction subscription replayed its earlier send");
         byte[] activeValue = new byte[] {0x41, 0x43, 0x54};
         byte[] activeTag = new byte[] {0x41, 0x31};
         send(publisher, publisherInteraction, publisherParameter, activeValue, activeTag);
         awaitCount(subscriber, subscriberRecorder, 1,
            "active interaction subscription delivery did not arrive before deadline");
         assertInteraction(subscriberRecorder.interactions.get(0), subscriberInteraction,
            subscriberParameter, activeValue, activeTag, producer, subscriber,
            "active interaction subscription");

         subscriberRecorder.interactions.clear();
         subscriber.subscribeInteractionClassPassively(subscriberInteraction);
         byte[] downgradedValue = new byte[] {0x44, 0x4f, 0x57};
         byte[] downgradedTag = new byte[] {0x44, 0x32};
         send(publisher, publisherInteraction, publisherParameter, downgradedValue, downgradedTag);
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberRecorder.interactions.isEmpty(),
            "passive interaction downgrade did not suppress delivery");

         subscriber.unsubscribeInteractionClass(subscriberInteraction);
         subscribed = false;
         byte[] withdrawnValue = new byte[] {0x57, 0x49, 0x54};
         byte[] withdrawnTag = new byte[] {0x57, 0x33};
         send(publisher, publisherInteraction, publisherParameter, withdrawnValue, withdrawnTag);
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberRecorder.interactions.isEmpty(),
            "unsubscribed interaction class unexpectedly received an interaction");

         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
      } finally {
         if (subscribed && subscriberJoined) {
            try { subscriber.unsubscribeInteractionClass(subscriberInteraction); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
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

   private static void settle(RTIambassador ambassador, CallbackModel model) throws Exception {
      for (int pass = 0; pass < 8; ++pass) JavaTckSupport.drain(ambassador);
      if (model == CallbackModel.HLA_IMMEDIATE) Thread.yield();
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
         description + " did not preserve class, parameter, value, tag, producer, "
            + "reliable transportation, and empty region metadata");
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
                  if ("toString".equals(name)) return "Java TCK interaction subscription lifecycle";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
