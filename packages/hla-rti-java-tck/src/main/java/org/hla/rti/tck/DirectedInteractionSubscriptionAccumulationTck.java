package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Sequential directed-interaction subscriptions accumulate on one object class. */
final class DirectedInteractionSubscriptionAccumulationTck {
   private static final String SIBLING_INTERACTION_CLASS_NAME =
      "HLAinteractionRoot.TckTypedAccumulationSiblingInteraction";

   private DirectedInteractionSubscriptionAccumulationTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-directed-subscription-accumulation-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      try {
         publisher.connect(new Recorder().proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "java-tck-directed-subscription-accumulation-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution(
            "java-tck-directed-subscription-accumulation-subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherIdentity = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle subscriberIdentity = subscriber.getAttributeHandle(
            subscriberClass, attributeName);
         InteractionClassHandle publisherFirstInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle publisherSecondInteraction =
            publisher.getInteractionClassHandle(SIBLING_INTERACTION_CLASS_NAME);
         InteractionClassHandle subscriberFirstInteraction =
            subscriber.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle subscriberSecondInteraction =
            subscriber.getInteractionClassHandle(SIBLING_INTERACTION_CLASS_NAME);
         ParameterHandle publisherFirstParameter = publisher.getParameterHandle(
            publisherFirstInteraction, "IntegerParameter");
         ParameterHandle publisherSecondParameter = publisher.getParameterHandle(
            publisherSecondInteraction, "SiblingParameter");
         ParameterHandle subscriberFirstParameter = subscriber.getParameterHandle(
            subscriberFirstInteraction, "IntegerParameter");
         ParameterHandle subscriberSecondParameter = subscriber.getParameterHandle(
            subscriberSecondInteraction, "SiblingParameter");
         JavaTckSupport.check(publisherClass.equals(subscriberClass)
               && publisherIdentity.equals(subscriberIdentity)
               && publisherFirstInteraction.equals(subscriberFirstInteraction)
               && publisherSecondInteraction.equals(subscriberSecondInteraction),
            "directed-subscription accumulation handles changed across federates");

         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherIdentity);
         AttributeHandleSet subscriberAttributes =
            JavaTckSupport.attributeSet(subscriber, subscriberIdentity);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);

         InteractionClassHandleSet published = JavaTckSupport.interactionSet(
            publisher, publisherFirstInteraction, publisherSecondInteraction);
         InteractionClassHandleSet firstSelection =
            JavaTckSupport.interactionSet(subscriber, subscriberFirstInteraction);
         InteractionClassHandleSet secondSelection =
            JavaTckSupport.interactionSet(subscriber, subscriberSecondInteraction);
         publisher.publishObjectClassDirectedInteractions(publisherClass, published);
         subscriber.subscribeObjectClassDirectedInteractions(subscriberClass, firstSelection);
         subscriber.subscribeObjectClassDirectedInteractions(subscriberClass, secondSelection);

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.drain(subscriber);
         JavaTckSupport.check(subscriberRecorder.discoveries.contains(target),
            "subscriber did not discover the directed-interaction target object");

         byte[] firstParameterBytes = new byte[] {0x31};
         byte[] secondParameterBytes = new byte[] {0x32};
         ParameterHandleValueMap firstParameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         firstParameters.put(publisherFirstParameter, firstParameterBytes);
         ParameterHandleValueMap secondParameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         secondParameters.put(publisherSecondParameter, secondParameterBytes);
         byte[] firstTag = new byte[] {0x53, 0x55, 0x42, 0x31};
         byte[] secondTag = new byte[] {0x53, 0x55, 0x42, 0x32};
         publisher.sendDirectedInteraction(publisherFirstInteraction, target,
            firstParameters, firstTag);
         publisher.sendDirectedInteraction(publisherSecondInteraction, target,
            secondParameters, secondTag);
         JavaTckSupport.drain(subscriber);

         JavaTckSupport.check(subscriberRecorder.directed.size() == 2,
            "sequential subscriptions did not deliver exactly both selected interaction classes");
         assertDelivery(subscriber, findDelivery(subscriberRecorder, subscriberFirstInteraction),
            subscriberFirstInteraction, target, subscriberFirstParameter,
            firstParameterBytes, firstTag, publisherFederate);
         assertDelivery(subscriber, findDelivery(subscriberRecorder, subscriberSecondInteraction),
            subscriberSecondInteraction, target, subscriberSecondParameter,
            secondParameterBytes, secondTag, publisherFederate);

         subscriber.unsubscribeObjectClassDirectedInteractions(subscriberClass, firstSelection);
         subscriber.unsubscribeObjectClassDirectedInteractions(subscriberClass, secondSelection);
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         publisher.unpublishObjectClassDirectedInteractions(publisherClass, published);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
      } finally {
         if (subscriberJoined) resign(subscriber);
         if (publisherJoined) resign(publisher);
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (subscriberConnected) disconnect(subscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static Directed findDelivery(Recorder recorder, InteractionClassHandle interaction) {
      for (Directed delivery : recorder.directed) {
         if (interaction.equals(delivery.interaction)) {
            return delivery;
         }
      }
      throw new AssertionError("directed callback omitted interaction " + interaction);
   }

   private static void assertDelivery(RTIambassador inspector, Directed delivery,
         InteractionClassHandle interaction, ObjectInstanceHandle target,
         ParameterHandle parameter, byte[] parameterBytes, byte[] tag,
         FederateHandle publisher) throws Exception {
      JavaTckSupport.check(interaction.equals(delivery.interaction),
         "directed callback carried the wrong interaction class");
      JavaTckSupport.check(target.equals(delivery.target),
         "directed callback carried the wrong target object");
      JavaTckSupport.check(delivery.parameters.size() == 1
            && Arrays.equals(parameterBytes, delivery.parameters.get(parameter)),
         "directed callback did not preserve its class-specific parameter value");
      JavaTckSupport.check(Arrays.equals(tag, delivery.tag),
         "directed callback did not preserve its tag");
      JavaTckSupport.check(delivery.tag != tag,
         "directed callback reused the sender's mutable tag array");
      JavaTckSupport.check(delivery.transportation != null
            && "HLAreliable".equals(inspector.getTransportationTypeName(delivery.transportation)),
         "directed callback did not preserve reliable transportation");
      JavaTckSupport.check(publisher.equals(delivery.producer),
         "directed callback carried the wrong producing federate");
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (Exception ignored) {
      }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
   }

   private static final class Directed {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle target;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;

      private Directed(InteractionClassHandle interaction, ObjectInstanceHandle target,
            ParameterHandleValueMap parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer) {
         this.interaction = interaction;
         this.target = target;
         this.parameters = parameters;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Directed> directed = new ArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveDirectedInteraction".equals(name)) {
                  ParameterHandleValueMap parameters =
                     ((ParameterHandleValueMap) arguments[2]).clone();
                  byte[] tag = ((byte[]) arguments[3]).clone();
                  directed.add(new Directed((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1], parameters, tag,
                     (TransportationTypeHandle) arguments[4],
                     (FederateHandle) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed subscription recorder";
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
