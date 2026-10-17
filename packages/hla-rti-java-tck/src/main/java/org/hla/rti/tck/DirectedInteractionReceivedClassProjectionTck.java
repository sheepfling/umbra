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

/** Verifies the received interaction class and parameter projection. */
final class DirectedInteractionReceivedClassProjectionTck {
   private DirectedInteractionReceivedClassProjectionTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String derivedObjectClassName, String identityAttributeName,
         String interactionClassName, String derivedInteractionClassName,
         String integerParameterName, String derivedParameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador superclassSubscriber = factory.getRtiAmbassador();
      RTIambassador sentClassSubscriber = factory.getRtiAmbassador();
      Recorder superclassRecorder = new Recorder();
      Recorder sentClassRecorder = new Recorder();
      String federation = "java-tck-directed-received-class-projection-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean superclassConnected = false;
      boolean sentClassConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean superclassJoined = false;
      boolean sentClassJoined = false;
      try {
         publisher.connect(new Recorder().proxy(), callbackModel);
         publisherConnected = true;
         superclassSubscriber.connect(superclassRecorder.proxy(), callbackModel);
         superclassConnected = true;
         sentClassSubscriber.connect(sentClassRecorder.proxy(), callbackModel);
         sentClassConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "java-tck-directed-received-class-projection-publisher", federation);
         publisherJoined = true;
         superclassSubscriber.joinFederationExecution(
            "java-tck-directed-received-class-projection-superclass", federation);
         superclassJoined = true;
         sentClassSubscriber.joinFederationExecution(
            "java-tck-directed-received-class-projection-sent-class", federation);
         sentClassJoined = true;

         ObjectClassHandle publisherObjectClass =
            publisher.getObjectClassHandle(derivedObjectClassName);
         ObjectClassHandle superclassObjectClass =
            superclassSubscriber.getObjectClassHandle(derivedObjectClassName);
         ObjectClassHandle sentClassObjectClass =
            sentClassSubscriber.getObjectClassHandle(derivedObjectClassName);
         InteractionClassHandle publisherBaseInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle publisherDerivedInteraction =
            publisher.getInteractionClassHandle(derivedInteractionClassName);
         InteractionClassHandle superclassBaseInteraction =
            superclassSubscriber.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle sentClassDerivedInteraction =
            sentClassSubscriber.getInteractionClassHandle(derivedInteractionClassName);
         JavaTckSupport.check(publisherObjectClass.equals(superclassObjectClass)
               && publisherObjectClass.equals(sentClassObjectClass)
               && publisherBaseInteraction.equals(superclassBaseInteraction)
               && publisherDerivedInteraction.equals(sentClassDerivedInteraction),
            "directed received-class handles changed across federates");

         AttributeHandle publisherIdentity = publisher.getAttributeHandle(
            publisherObjectClass, identityAttributeName);
         AttributeHandle superclassIdentity = superclassSubscriber.getAttributeHandle(
            superclassObjectClass, identityAttributeName);
         AttributeHandle sentClassIdentity = sentClassSubscriber.getAttributeHandle(
            sentClassObjectClass, identityAttributeName);
         ParameterHandle publisherBaseParameter = publisher.getParameterHandle(
            publisherBaseInteraction, integerParameterName);
         ParameterHandle publisherDerivedBaseParameter = publisher.getParameterHandle(
            publisherDerivedInteraction, integerParameterName);
         ParameterHandle publisherDerivedParameter = publisher.getParameterHandle(
            publisherDerivedInteraction, derivedParameterName);
         ParameterHandle superclassBaseParameter = superclassSubscriber.getParameterHandle(
            superclassBaseInteraction, integerParameterName);
         ParameterHandle sentClassBaseParameter = sentClassSubscriber.getParameterHandle(
            sentClassDerivedInteraction, integerParameterName);
         ParameterHandle sentClassDerivedParameter = sentClassSubscriber.getParameterHandle(
            sentClassDerivedInteraction, derivedParameterName);
         JavaTckSupport.check(publisherIdentity.equals(superclassIdentity)
               && publisherIdentity.equals(sentClassIdentity)
               && publisherBaseParameter.equals(publisherDerivedBaseParameter)
               && publisherBaseParameter.equals(superclassBaseParameter)
               && publisherBaseParameter.equals(sentClassBaseParameter)
               && publisherDerivedParameter.equals(sentClassDerivedParameter),
            "directed received-class member handles changed across federates");

         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherIdentity);
         AttributeHandleSet superclassAttributes =
            JavaTckSupport.attributeSet(superclassSubscriber, superclassIdentity);
         AttributeHandleSet sentClassAttributes =
            JavaTckSupport.attributeSet(sentClassSubscriber, sentClassIdentity);
         publisher.publishObjectClassAttributes(publisherObjectClass, publisherAttributes);
         superclassSubscriber.subscribeObjectClassAttributes(
            superclassObjectClass, superclassAttributes);
         sentClassSubscriber.subscribeObjectClassAttributes(
            sentClassObjectClass, sentClassAttributes);

         InteractionClassHandleSet published = JavaTckSupport.interactionSet(
            publisher, publisherBaseInteraction, publisherDerivedInteraction);
         InteractionClassHandleSet superclassSelection = JavaTckSupport.interactionSet(
            superclassSubscriber, superclassBaseInteraction);
         InteractionClassHandleSet sentClassSelection = JavaTckSupport.interactionSet(
            sentClassSubscriber, sentClassDerivedInteraction);
         publisher.publishObjectClassDirectedInteractions(publisherObjectClass, published);
         superclassSubscriber.subscribeObjectClassDirectedInteractions(
            superclassObjectClass, superclassSelection);
         sentClassSubscriber.subscribeObjectClassDirectedInteractions(
            sentClassObjectClass, sentClassSelection);

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherObjectClass);
         JavaTckSupport.drain(superclassSubscriber);
         JavaTckSupport.drain(sentClassSubscriber);
         JavaTckSupport.check(superclassRecorder.discoveries.contains(target)
               && sentClassRecorder.discoveries.contains(target),
            "directed received-class subscribers did not discover the target object");

         byte[] baseValue = new byte[] {0x51};
         byte[] derivedValue = new byte[] {0x72};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(2);
         parameters.put(publisherDerivedBaseParameter, baseValue);
         parameters.put(publisherDerivedParameter, derivedValue);
         byte[] tag = new byte[] {0x44, 0x49, 0x52, 0x50};
         publisher.sendDirectedInteraction(
            publisherDerivedInteraction, target, parameters, tag);
         JavaTckSupport.drain(superclassSubscriber);
         JavaTckSupport.drain(sentClassSubscriber);

         JavaTckSupport.check(superclassRecorder.directed.size() == 1,
            "superclass subscriber did not receive exactly one directed interaction");
         JavaTckSupport.check(sentClassRecorder.directed.size() == 1,
            "sent-class subscriber did not receive exactly one directed interaction");
         assertSuperclassDelivery(superclassSubscriber, superclassRecorder.directed.get(0),
            superclassBaseInteraction, target, superclassBaseParameter, publisherDerivedParameter,
            baseValue, tag, publisherFederate);
         assertSentClassDelivery(sentClassSubscriber, sentClassRecorder.directed.get(0),
            sentClassDerivedInteraction, target, sentClassBaseParameter,
            sentClassDerivedParameter, baseValue, derivedValue, tag, publisherFederate);

         superclassSubscriber.unsubscribeObjectClassDirectedInteractions(
            superclassObjectClass, superclassSelection);
         sentClassSubscriber.unsubscribeObjectClassDirectedInteractions(
            sentClassObjectClass, sentClassSelection);
         superclassSubscriber.unsubscribeObjectClassAttributes(
            superclassObjectClass, superclassAttributes);
         sentClassSubscriber.unsubscribeObjectClassAttributes(
            sentClassObjectClass, sentClassAttributes);
         publisher.unpublishObjectClassDirectedInteractions(publisherObjectClass, published);
         publisher.unpublishObjectClassAttributes(publisherObjectClass, publisherAttributes);
      } finally {
         if (sentClassJoined) resign(sentClassSubscriber);
         if (superclassJoined) resign(superclassSubscriber);
         if (publisherJoined) resign(publisher);
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (sentClassConnected) disconnect(sentClassSubscriber);
         if (superclassConnected) disconnect(superclassSubscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void assertSuperclassDelivery(RTIambassador inspector, Directed delivery,
         InteractionClassHandle expectedInteraction, ObjectInstanceHandle target,
         ParameterHandle baseParameter, ParameterHandle derivedParameter,
         byte[] baseValue, byte[] tag, FederateHandle producer) throws Exception {
      JavaTckSupport.check(expectedInteraction.equals(delivery.interaction)
            && target.equals(delivery.target),
         "directed callback did not report the closest subscribed superclass and target");
      JavaTckSupport.check(delivery.parameters.size() == 1
            && delivery.parameters.containsKey(baseParameter)
            && !delivery.parameters.containsKey(derivedParameter)
            && Arrays.equals(baseValue, delivery.parameters.get(baseParameter)),
         "directed superclass callback did not project only the inherited parameter");
      assertCommonDelivery(inspector, delivery, tag, producer);
   }

   private static void assertSentClassDelivery(RTIambassador inspector, Directed delivery,
         InteractionClassHandle expectedInteraction, ObjectInstanceHandle target,
         ParameterHandle baseParameter, ParameterHandle derivedParameter,
         byte[] baseValue, byte[] derivedValue, byte[] tag, FederateHandle producer)
         throws Exception {
      JavaTckSupport.check(expectedInteraction.equals(delivery.interaction)
            && target.equals(delivery.target),
         "directed callback did not retain its sent class and target");
      JavaTckSupport.check(delivery.parameters.size() == 2
            && delivery.parameters.containsKey(baseParameter)
            && delivery.parameters.containsKey(derivedParameter)
            && Arrays.equals(baseValue, delivery.parameters.get(baseParameter))
            && Arrays.equals(derivedValue, delivery.parameters.get(derivedParameter)),
         "directed exact-class callback did not preserve inherited and derived parameters");
      assertCommonDelivery(inspector, delivery, tag, producer);
   }

   private static void assertCommonDelivery(RTIambassador inspector, Directed delivery,
         byte[] tag, FederateHandle producer) throws Exception {
      JavaTckSupport.check(Arrays.equals(tag, delivery.tag) && delivery.tag != tag,
         "directed callback did not preserve a distinct copy of the sent tag");
      JavaTckSupport.check(producer.equals(delivery.producer),
         "directed callback carried the wrong producing federate");
      JavaTckSupport.check(delivery.transportation != null
            && "HLAreliable".equals(
               inspector.getTransportationTypeName(delivery.transportation)),
         "directed callback did not preserve reliable transportation");
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
                  if ("toString".equals(name)) return "Java TCK received-class recorder";
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
