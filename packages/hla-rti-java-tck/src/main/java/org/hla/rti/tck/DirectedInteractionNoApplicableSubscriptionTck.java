package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

/** Verifies that an unsubscribed receiver gets no directed interaction callback. */
final class DirectedInteractionNoApplicableSubscriptionTck {
   private DirectedInteractionNoApplicableSubscriptionTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String identityAttributeName,
         String interactionClassName, String integerParameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-directed-no-applicable-subscription-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      try {
         publisher.connect(new Recorder().proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution(
            "java-tck-directed-no-subscription-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(
            "java-tck-directed-no-subscription-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherObjectClass =
            publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverObjectClass =
            receiver.getObjectClassHandle(objectClassName);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(publisherObjectClass.equals(receiverObjectClass)
               && publisherInteraction.equals(receiverInteraction),
            "directed no-subscription class handles changed across federates");

         AttributeHandle publisherIdentity = publisher.getAttributeHandle(
            publisherObjectClass, identityAttributeName);
         AttributeHandle receiverIdentity = receiver.getAttributeHandle(
            receiverObjectClass, identityAttributeName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, integerParameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, integerParameterName);
         JavaTckSupport.check(publisherIdentity.equals(receiverIdentity)
               && publisherParameter.equals(receiverParameter),
            "directed no-subscription member handles changed across federates");

         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherIdentity);
         AttributeHandleSet receiverAttributes =
            JavaTckSupport.attributeSet(receiver, receiverIdentity);
         publisher.publishObjectClassAttributes(publisherObjectClass, publisherAttributes);
         receiver.subscribeObjectClassAttributes(receiverObjectClass, receiverAttributes);

         InteractionClassHandleSet publishedInteractions =
            JavaTckSupport.interactionSet(publisher, publisherInteraction);
         publisher.publishObjectClassDirectedInteractions(
            publisherObjectClass, publishedInteractions);

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherObjectClass);
         long discoveryDeadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(5);
         while (!receiverRecorder.discoveries.contains(target)
               && System.nanoTime() < discoveryDeadline) {
            receiver.evokeCallback(0.01);
         }
         JavaTckSupport.check(receiverRecorder.discoveries.contains(target),
            "receiver did not discover the known directed-interaction target");

         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, new byte[] {0x4e});
         publisher.sendDirectedInteraction(
            publisherInteraction, target, parameters, new byte[] {0x4e, 0x4f, 0x53, 0x55});

         long noDeliveryDeadline = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(500);
         while (System.nanoTime() < noDeliveryDeadline) {
            receiver.evokeCallback(0.01);
         }
         JavaTckSupport.check(receiverRecorder.directed.isEmpty()
               && receiverRecorder.ordinaryInteractionCallbacks == 0,
            "receiver without an applicable directed-interaction subscription received an interaction callback");

         receiver.unsubscribeObjectClassAttributes(receiverObjectClass, receiverAttributes);
         publisher.unpublishObjectClassDirectedInteractions(
            publisherObjectClass, publishedInteractions);
         publisher.unpublishObjectClassAttributes(publisherObjectClass, publisherAttributes);
      } finally {
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
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

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<Object> directed = new ArrayList<>();
      private int ordinaryInteractionCallbacks;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveDirectedInteraction".equals(name)) {
                  directed.add(arguments[0]);
               } else if ("receiveInteraction".equals(name)) {
                  ++ordinaryInteractionCallbacks;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK no-subscription recorder";
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
