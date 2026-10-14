package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies ordinary interaction delivery to active and passive subscribers. */
final class InteractionServiceBoundariesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private InteractionServiceBoundariesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador active = factory.getRtiAmbassador();
      RTIambassador passive = factory.getRtiAmbassador();
      Recorder activeCallbacks = new Recorder();
      Recorder passiveCallbacks = new Recorder();
      String federation = "java-tck-interaction-service-boundaries-" + UUID.randomUUID();
      boolean publisherConnected = false, activeConnected = false, passiveConnected = false;
      boolean created = false, publisherJoined = false, activeJoined = false, passiveJoined = false;
      boolean published = false, activeSubscribed = false, passiveSubscribed = false;
      try {
         publisher.connect(callbacks(), callbackModel);
         publisherConnected = true;
         active.connect(activeCallbacks.proxy(), callbackModel);
         activeConnected = true;
         passive.connect(passiveCallbacks.proxy(), callbackModel);
         passiveConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-interaction-boundary-publisher", federation);
         publisherJoined = true;
         active.joinFederationExecution("java-tck-interaction-boundary-active", federation);
         activeJoined = true;
         passive.joinFederationExecution("java-tck-interaction-boundary-passive", federation);
         passiveJoined = true;

         InteractionClassHandle publisherClass = publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle activeClass = active.getInteractionClassHandle(interactionName);
         InteractionClassHandle passiveClass = passive.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherClass, parameterName);
         ParameterHandle activeParameter = active.getParameterHandle(activeClass, parameterName);
         ParameterHandle passiveParameter = passive.getParameterHandle(passiveClass, parameterName);
         JavaTckSupport.check(interactionName.equals(publisher.getInteractionClassName(publisherClass))
               && interactionName.equals(active.getInteractionClassName(activeClass))
               && interactionName.equals(passive.getInteractionClassName(passiveClass)),
            "adapter FOM did not resolve the ordinary interaction class consistently");

         publisher.publishInteractionClass(publisherClass);
         published = true;
         active.subscribeInteractionClass(activeClass);
         activeSubscribed = true;
         passive.subscribeInteractionClassPassively(passiveClass);
         passiveSubscribed = true;

         byte[] value = new byte[] {0x42, 0x11};
         byte[] tag = new byte[] {0x49, 0x4e, 0x54};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, value.clone());
         publisher.sendInteraction(publisherClass, parameters, tag.clone());

         await(active, callbackModel, activeCallbacks,
            "active interaction subscriber did not receive delivery");
         assertDelivery(activeCallbacks.interactions.get(0), activeClass, activeParameter,
            value, tag, producer, active, "active interaction subscriber");

         for (int pass = 0; pass < 8; ++pass) settle(passive, callbackModel);
         JavaTckSupport.check(!passiveCallbacks.interactions.isEmpty(),
            "passive interaction subscription did not receive the subscribed delivery");
         assertDelivery(passiveCallbacks.interactions.get(0), passiveClass, passiveParameter,
            value, tag, producer, passive, "passive interaction subscriber");
      } finally {
         if (passiveJoined && passiveSubscribed) {
            try { passive.unsubscribeInteractionClass(passive.getInteractionClassHandle(interactionName)); }
            catch (Exception ignored) { }
         }
         if (activeJoined && activeSubscribed) {
            try { active.unsubscribeInteractionClass(active.getInteractionClassHandle(interactionName)); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published) {
            try { publisher.unpublishInteractionClass(publisher.getInteractionClassHandle(interactionName)); }
            catch (Exception ignored) { }
         }
         if (passiveJoined) resign(passive);
         if (activeJoined) resign(active);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (passiveConnected) disconnect(passive);
         if (activeConnected) disconnect(active);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void assertDelivery(Interaction event, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] value, byte[] tag, FederateHandle producer,
         RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.parameters.size() == 1 && Arrays.equals(value, event.parameters.get(parameter))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty()
            && event.regionsEmpty,
         description + " returned incorrect class, parameter, payload, tag, producer, or metadata");
   }

   private static void await(RTIambassador ambassador, CallbackModel model, Recorder recorder,
         String failure) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && recorder.interactions.isEmpty()) {
         settle(ambassador, model);
      }
      JavaTckSupport.check(recorder.interactions.size() == 1, failure);
   }

   private static void settle(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Interaction(InteractionClassHandle interaction, ParameterHandleValueMap parameters,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.interaction = interaction;
         this.parameters = parameters.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regions == null || regions.isEmpty();
      }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments != null && arguments.length == 6) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK interaction service boundary callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }
}
