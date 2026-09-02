package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
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

/** P1.4 ordinary directed interaction publication, selection, and delivery. */
final class DirectedInteractionsTck {
   private DirectedInteractionsTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador targeted = factory.getRtiAmbassador();
      RTIambassador universal = factory.getRtiAmbassador();
      RTIambassador unsubscribed = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder targetedRecorder = new Recorder();
      Recorder universalRecorder = new Recorder();
      Recorder unsubscribedRecorder = new Recorder();
      String federation = "java-tck-directed-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean targetedConnected = false;
      boolean universalConnected = false;
      boolean unsubscribedConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean targetedJoined = false;
      boolean universalJoined = false;
      boolean unsubscribedJoined = false;
      try {
         publisher.connect(publisherRecorder.proxy(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         targeted.connect(targetedRecorder.proxy(), CallbackModel.HLA_EVOKED);
         targetedConnected = true;
         universal.connect(universalRecorder.proxy(), CallbackModel.HLA_IMMEDIATE);
         universalConnected = true;
         unsubscribed.connect(unsubscribedRecorder.proxy(), CallbackModel.HLA_EVOKED);
         unsubscribedConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "java-tck-directed-publisher", federation);
         publisherJoined = true;
         targeted.joinFederationExecution("java-tck-directed-targeted", federation);
         targetedJoined = true;
         universal.joinFederationExecution("java-tck-directed-universal", federation);
         universalJoined = true;
         unsubscribed.joinFederationExecution("java-tck-directed-none", federation);
         unsubscribedJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle targetedClass = targeted.getObjectClassHandle(objectClassName);
         ObjectClassHandle universalClass = universal.getObjectClassHandle(objectClassName);
         ObjectClassHandle unsubscribedClass = unsubscribed.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         AttributeHandle targetedAttribute = targeted.getAttributeHandle(targetedClass,
            attributeName);
         AttributeHandle universalAttribute = universal.getAttributeHandle(universalClass,
            attributeName);
         AttributeHandle unsubscribedAttribute = unsubscribed.getAttributeHandle(
            unsubscribedClass, attributeName);
         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(publisher,
            publisherAttribute);
         AttributeHandleSet targetedAttributes = JavaTckSupport.attributeSet(targeted,
            targetedAttribute);
         AttributeHandleSet universalAttributes = JavaTckSupport.attributeSet(universal,
            universalAttribute);
         AttributeHandleSet unsubscribedAttributes = JavaTckSupport.attributeSet(
            unsubscribed, unsubscribedAttribute);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle targetedInteraction =
            targeted.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle universalInteraction =
            universal.getInteractionClassHandle(interactionClassName);

         // Publish the object class from both potential target owners.  The
         // publisher subscribes to the class so it can address the targeted
         // member's object through a globally known handle.
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         targeted.publishObjectClassAttributes(targetedClass, targetedAttributes);
         publisher.subscribeObjectClassAttributes(publisherClass, publisherAttributes);
         targeted.subscribeObjectClassAttributes(targetedClass, targetedAttributes);
         universal.subscribeObjectClassAttributes(universalClass, universalAttributes);
         unsubscribed.subscribeObjectClassAttributes(unsubscribedClass,
            unsubscribedAttributes);

         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         targeted.subscribeObjectClassDirectedInteractions(targetedClass,
            JavaTckSupport.interactionSet(targeted, targetedInteraction));
         universal.subscribeObjectClassDirectedInteractionsUniversally(universalClass,
            JavaTckSupport.interactionSet(universal, universalInteraction));

         ObjectInstanceHandle publisherTarget = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle targetedTarget = targeted.registerObjectInstance(targetedClass);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(targeted);
         JavaTckSupport.drain(unsubscribed);
         JavaTckSupport.check(publisher.getKnownObjectClassHandle(targetedTarget).equals(
               publisherClass), "publisher did not learn the targeted object's class");
         JavaTckSupport.check(targetedRecorder.discoveries.contains(publisherTarget),
            "targeted member did not discover the publisher-owned object");
         JavaTckSupport.check(universalRecorder.discoveries.contains(publisherTarget),
            "universal member did not discover the publisher-owned object");

         ParameterHandleValueMap empty = publisher.getParameterHandleValueMapFactory().create(0);
         byte[] targetedTag = new byte[] {0x11, 0x22, 0x33};
         publisher.sendDirectedInteraction(publisherInteraction, targetedTarget, empty,
            targetedTag);
         JavaTckSupport.check(universalRecorder.directed.size() == 1,
            "universal directed subscriber did not receive the targeted send immediately");
         JavaTckSupport.check(targetedRecorder.directed.size() == 0,
            "evoked targeted subscriber received directed data before callback drain");
         JavaTckSupport.drain(targeted);
         JavaTckSupport.check(targetedRecorder.directed.size() == 1,
            "by-ownership directed subscriber did not receive its targeted send");
         assertDirected(targeted, targetedRecorder.directed.get(0), targetedInteraction,
            targetedTarget, targetedTag, publisherFederate);
         assertDirected(universal, universalRecorder.directed.get(0), universalInteraction,
            targetedTarget, targetedTag, publisherFederate);
         JavaTckSupport.check(unsubscribedRecorder.directed.isEmpty(),
            "unsubscribed member received a directed interaction");

         byte[] publisherTag = new byte[] {0x44};
         publisher.sendDirectedInteraction(publisherInteraction, publisherTarget,
            publisher.getParameterHandleValueMapFactory().create(0), publisherTag);
         JavaTckSupport.check(universalRecorder.directed.size() == 2,
            "universal directed subscriber did not receive a second target");
         JavaTckSupport.drain(targeted);
         JavaTckSupport.check(targetedRecorder.directed.size() == 1,
            "by-ownership directed subscriber received another owner's target");
         assertDirected(universal, universalRecorder.directed.get(1), universalInteraction,
            publisherTarget, publisherTag, publisherFederate);

         // Selective and whole unsubscribe are both ordinary declaration
         // boundaries; after removal no queued or subsequent send is routed.
         targeted.unsubscribeObjectClassDirectedInteractions(targetedClass,
            JavaTckSupport.interactionSet(targeted, targetedInteraction));
         targeted.unsubscribeObjectClassDirectedInteractions(targetedClass,
            JavaTckSupport.interactionSet(targeted, targetedInteraction));
         universal.unsubscribeObjectClassDirectedInteractions(universalClass);
         publisher.sendDirectedInteraction(publisherInteraction, targetedTarget,
            publisher.getParameterHandleValueMapFactory().create(0), new byte[] {0x55});
         JavaTckSupport.drain(targeted);
         JavaTckSupport.check(targetedRecorder.directed.size() == 1,
            "unsubscribed targeted member received a later directed interaction");
         JavaTckSupport.check(universalRecorder.directed.size() == 2,
            "unsubscribed universal member received a later directed interaction");

         publisher.unpublishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         publisher.unpublishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         JavaTckSupport.expectFailure(
            () -> publisher.sendDirectedInteraction(publisherInteraction, targetedTarget,
               publisher.getParameterHandleValueMapFactory().create(0), new byte[0]),
            "InteractionClassNotPublished", "directed send after unpublication was accepted");
         JavaTckSupport.expectFailure(
            () -> publisher.getObjectInstanceHandle("java-tck-no-such-directed-object"),
            "ObjectInstanceNotKnown", "unknown directed target lookup was accepted");

         targeted.unsubscribeObjectClass(targetedClass);
         universal.unsubscribeObjectClass(universalClass);
         unsubscribed.unsubscribeObjectClass(unsubscribedClass);
         publisher.unsubscribeObjectClass(publisherClass);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         targeted.unpublishObjectClassAttributes(targetedClass, targetedAttributes);
         publisher.unpublishObjectClass(publisherClass);
         targeted.unpublishObjectClass(targetedClass);
      } finally {
         if (unsubscribedJoined) resign(unsubscribed);
         if (universalJoined) resign(universal);
         if (targetedJoined) resign(targeted);
         if (publisherJoined) resign(publisher);
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (unsubscribedConnected) disconnect(unsubscribed);
         if (universalConnected) disconnect(universal);
         if (targetedConnected) disconnect(targeted);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void assertDirected(RTIambassador inspector, Directed directed,
         InteractionClassHandle interaction, ObjectInstanceHandle target, byte[] tag,
         FederateHandle publisher) throws Exception {
      JavaTckSupport.check(interaction.equals(directed.interaction),
         "directed callback carried the wrong interaction class");
      JavaTckSupport.check(target.equals(directed.target),
         "directed callback carried the wrong target object");
      JavaTckSupport.check(directed.parameters.isEmpty(),
         "directed callback unexpectedly carried parameters");
      JavaTckSupport.check(Arrays.equals(tag, directed.tag),
         "directed callback did not preserve the tag bytes");
      JavaTckSupport.check(directed.tag != tag,
         "directed callback reused the sender's mutable tag array");
      JavaTckSupport.check(directed.transportation != null
            && "HLAreliable".equals(inspector.getTransportationTypeName(directed.transportation)),
         "directed callback did not preserve the reliable transportation type");
      JavaTckSupport.check(publisher.equals(directed.producingFederate),
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
      private final FederateHandle producingFederate;

      private Directed(InteractionClassHandle interaction, ObjectInstanceHandle target,
            ParameterHandleValueMap parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producingFederate) {
         this.interaction = interaction;
         this.target = target;
         this.parameters = parameters;
         this.tag = tag;
         this.transportation = transportation;
         this.producingFederate = producingFederate;
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
               if ("receiveDirectedInteraction".equals(name)) {
                  ParameterHandleValueMap parameters =
                     ((ParameterHandleValueMap) arguments[2]).clone();
                  byte[] tag = ((byte[]) arguments[3]).clone();
                  TransportationTypeHandle transportation =
                     (TransportationTypeHandle) arguments[4];
                  directed.add(new Directed((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1], parameters, tag, transportation,
                     (FederateHandle) arguments[5]));
               } else if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed callbacks";
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
