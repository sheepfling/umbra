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

/** Directed-interaction delivery, callback ordering, and target deletion lifecycle. */
final class DirectedInteractionTargetLifecycleTck {
   private DirectedInteractionTargetLifecycleTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-directed-target-lifecycle-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean publisherAttributesPublished = false, receiverAttributesSubscribed = false;
      boolean interactionsPublished = false, interactionsSubscribed = false;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandleSet publisherAttributes = null, receiverAttributes = null;
      InteractionClassHandle publisherInteraction = null, receiverInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-target-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-directed-target-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle sendInteraction = publisherInteraction;

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisherAttributesPublished = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         receiverAttributesSubscribed = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         interactionsPublished = true;
         receiver.subscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         interactionsSubscribed = true;

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         await(receiver, () -> receiverRecorder.discoveries.contains(target),
            "directed target was not discovered");
         JavaTckSupport.check(receiver.getKnownObjectClassHandle(target).equals(receiverClass),
            "receiver reported the wrong known class for the directed target");

         byte[] firstTag = new byte[] {0x41, 0x11};
         publisher.sendDirectedInteraction(publisherInteraction, target,
            publisher.getParameterHandleValueMapFactory().create(0), firstTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(receiverRecorder.directed.isEmpty(),
               "evoked directed callback arrived before callback servicing");
         }
         await(receiver, () -> receiverRecorder.directed.size() == 1,
            "directed interaction was not delivered to the object target");
         assertDirected(receiver, receiverRecorder.directed.get(0), receiverInteraction,
            target, firstTag, producer);

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            int delivered = receiverRecorder.directed.size();
            byte[] queuedAtUnsubscribe = new byte[] {0x42, 0x22};
            publisher.sendDirectedInteraction(publisherInteraction, target,
               publisher.getParameterHandleValueMapFactory().create(0), queuedAtUnsubscribe);
            receiver.unsubscribeObjectClassDirectedInteractions(receiverClass,
               JavaTckSupport.interactionSet(receiver, receiverInteraction));
            JavaTckSupport.drain(receiver);
            JavaTckSupport.check(receiverRecorder.directed.size() == delivered,
               "queued directed data survived removal of its subscription");
            receiver.subscribeObjectClassDirectedInteractions(receiverClass,
               JavaTckSupport.interactionSet(receiver, receiverInteraction));

            byte[] afterResubscribe = new byte[] {0x43, 0x33};
            int afterResubscribeCount = delivered + 1;
            publisher.sendDirectedInteraction(publisherInteraction, target,
               publisher.getParameterHandleValueMapFactory().create(0), afterResubscribe);
            await(receiver, () -> receiverRecorder.directed.size() == afterResubscribeCount,
               "directed data was not delivered after resubscription");
            assertDirected(receiver, receiverRecorder.directed.get(delivered),
               receiverInteraction, target, afterResubscribe, producer);
            ++delivered;

            byte[] queuedAtUnpublication = new byte[] {0x44, 0x44};
            publisher.sendDirectedInteraction(publisherInteraction, target,
               publisher.getParameterHandleValueMapFactory().create(0), queuedAtUnpublication);
            publisher.unpublishObjectClassDirectedInteractions(publisherClass,
               JavaTckSupport.interactionSet(publisher, publisherInteraction));
            JavaTckSupport.drain(receiver);
            JavaTckSupport.check(receiverRecorder.directed.size() == delivered,
               "queued directed data survived source unpublication");
            publisher.publishObjectClassDirectedInteractions(publisherClass,
               JavaTckSupport.interactionSet(publisher, publisherInteraction));

            byte[] afterRepublication = new byte[] {0x45, 0x55};
            int afterRepublicationCount = delivered + 1;
            publisher.sendDirectedInteraction(publisherInteraction, target,
               publisher.getParameterHandleValueMapFactory().create(0), afterRepublication);
            await(receiver, () -> receiverRecorder.directed.size() == afterRepublicationCount,
               "directed data was not delivered after republication");
            assertDirected(receiver, receiverRecorder.directed.get(delivered),
               receiverInteraction, target, afterRepublication, producer);
         }

         byte[] deletionTag = new byte[] {0x46, 0x66};
         int deliveriesBeforeDeletion = receiverRecorder.directed.size();
         publisher.sendDirectedInteraction(publisherInteraction, target,
            publisher.getParameterHandleValueMapFactory().create(0), deletionTag);
         publisher.deleteObjectInstance(target, deletionTag);
         await(receiver, () -> receiverRecorder.removals.size() == 1,
            "target deletion callback was not delivered");
         int expectedDeliveries = deliveriesBeforeDeletion
            + (callbackModel == CallbackModel.HLA_IMMEDIATE ? 1 : 0);
         JavaTckSupport.check(receiverRecorder.directed.size() == expectedDeliveries,
            "directed callback crossed the target-deletion boundary unexpectedly");
         Removal removal = receiverRecorder.removals.get(0);
         JavaTckSupport.check(target.equals(removal.object)
               && Arrays.equals(deletionTag, removal.tag)
               && producer.equals(removal.producer),
            "removeObjectInstance reported incorrect target, tag, or producer");
         JavaTckSupport.expectFailure(
            () -> publisher.sendDirectedInteraction(sendInteraction, target,
               publisher.getParameterHandleValueMapFactory().create(0), new byte[0]),
            "ObjectInstanceNotKnown", "directed send after target deletion");
      } finally {
         if (interactionsSubscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassDirectedInteractions(receiverClass); }
            catch (Exception ignored) { }
         }
         if (interactionsPublished && publisherJoined) {
            try { publisher.unpublishObjectClassDirectedInteractions(publisherClass); }
            catch (Exception ignored) { }
         }
         if (receiverAttributesSubscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClass(receiverClass); }
            catch (Exception ignored) { }
         }
         if (publisherAttributesPublished && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
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

   private static void await(RTIambassador ambassador, Check condition, String message)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.ok()) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(condition.ok(), message);
   }

   private static void assertDirected(RTIambassador inspector, Directed received,
         InteractionClassHandle interaction, ObjectInstanceHandle target, byte[] tag,
         FederateHandle producer) throws Exception {
      JavaTckSupport.check(interaction.equals(received.interaction)
            && target.equals(received.target) && received.parameters.isEmpty(),
         "directed callback carried the wrong class, target, or parameters");
      JavaTckSupport.check(Arrays.equals(tag, received.tag) && received.tag != tag,
         "directed callback did not preserve an independent tag value");
      JavaTckSupport.check(received.transportation != null
            && "HLAreliable".equals(inspector.getTransportationTypeName(received.transportation)),
         "directed callback did not preserve reliable transportation");
      JavaTckSupport.check(producer.equals(received.producer),
         "directed callback carried the wrong producer");
   }

   @FunctionalInterface
   private interface Check {
      boolean ok() throws Exception;
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

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;

      private Removal(ObjectInstanceHandle object, byte[] tag, FederateHandle producer) {
         this.object = object;
         this.tag = tag;
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Directed> directed = new ArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<Removal> removals = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveDirectedInteraction".equals(name)) {
                  directed.add(new Directed((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1],
                     ((ParameterHandleValueMap) arguments[2]).clone(),
                     ((byte[]) arguments[3]).clone(),
                     (TransportationTypeHandle) arguments[4], (FederateHandle) arguments[5]));
               } else if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("removeObjectInstance".equals(name)) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     ((byte[]) arguments[1]).clone(), (FederateHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed target callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
