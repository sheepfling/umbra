package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Receive-order directed receipt is selected using the target's known object class. */
final class DirectedKnownObjectClassFallbackTck {
   private DirectedKnownObjectClassFallbackTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseObjectName, String childObjectName, String markerName,
         String baseInteractionName, String childInteractionName,
         String baseParameterName, String childParameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-directed-known-class-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "directed-known-class-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("directed-known-class-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle childObject = publisher.getObjectClassHandle(childObjectName);
         ObjectClassHandle baseObject = receiver.getObjectClassHandle(baseObjectName);
         ObjectClassHandle receiverChildObject = receiver.getObjectClassHandle(childObjectName);
         InteractionClassHandle sentClass = publisher.getInteractionClassHandle(childInteractionName);
         InteractionClassHandle baseClass = receiver.getInteractionClassHandle(baseInteractionName);
         InteractionClassHandle receiverChildClass = receiver.getInteractionClassHandle(childInteractionName);
         JavaTckSupport.check(!baseObject.equals(receiverChildObject)
               && !baseClass.equals(receiverChildClass)
               && sentClass.equals(receiverChildClass),
            "adapter FOM lacks the distinct directed object and interaction hierarchies");
         AttributeHandle publisherMarker = publisher.getAttributeHandle(childObject, markerName);
         AttributeHandle receiverMarker = receiver.getAttributeHandle(baseObject, markerName);
         ParameterHandle sentBaseParameter = publisher.getParameterHandle(
            sentClass, baseParameterName);
         ParameterHandle sentChildParameter = publisher.getParameterHandle(
            sentClass, childParameterName);
         ParameterHandle receivedBaseParameter = receiver.getParameterHandle(
            baseClass, baseParameterName);
         JavaTckSupport.check(publisherMarker.equals(receiverMarker)
               && sentBaseParameter.equals(receivedBaseParameter),
            "directed fixture member handles changed across federates");

         publisher.publishObjectClassAttributes(childObject,
            JavaTckSupport.attributeSet(publisher, publisherMarker));
         receiver.subscribeObjectClassAttributes(baseObject,
            JavaTckSupport.attributeSet(receiver, receiverMarker));
         publisher.publishObjectClassDirectedInteractions(childObject,
            JavaTckSupport.interactionSet(publisher, sentClass));
         receiver.subscribeObjectClassDirectedInteractions(baseObject,
            JavaTckSupport.interactionSet(receiver, baseClass));
         receiver.subscribeObjectClassDirectedInteractions(receiverChildObject,
            JavaTckSupport.interactionSet(receiver, receiverChildClass));

         ObjectInstanceHandle target = publisher.registerObjectInstance(childObject);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.discoveries.size() == 1
               && target.equals(receiverRecorder.discoveries.get(0).object)
               && baseObject.equals(receiverRecorder.discoveries.get(0).objectClass)
               && baseObject.equals(receiver.getKnownObjectClassHandle(target)),
            "receiver did not know the child-registered target only at the base object class");

         byte[] baseValue = new byte[] {0x42};
         byte[] childValue = new byte[] {0x43};
         byte[] tag = new byte[] {0x44, 0x4b, 0x43};
         ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory().create(2);
         parameters.put(sentBaseParameter, baseValue);
         parameters.put(sentChildParameter, childValue);
         publisher.sendDirectedInteraction(sentClass, target, parameters, tag);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.directed.size() == 1,
            "known-class directed subscriber did not receive exactly one callback");
         Directed delivery = receiverRecorder.directed.get(0);
         JavaTckSupport.check(baseClass.equals(delivery.interaction)
               && target.equals(delivery.target)
               && producer.equals(delivery.producer)
               && delivery.parameters.size() == 1
               && Arrays.equals(baseValue, delivery.parameters.get(receivedBaseParameter))
               && !delivery.parameters.containsKey(sentChildParameter)
               && Arrays.equals(tag, delivery.tag),
            "directed receipt did not select the base known-class subscription and parameter view");

         receiver.unsubscribeObjectClassDirectedInteractions(receiverChildObject,
            JavaTckSupport.interactionSet(receiver, receiverChildClass));
         receiver.unsubscribeObjectClassDirectedInteractions(baseObject,
            JavaTckSupport.interactionSet(receiver, baseClass));
         receiver.unsubscribeObjectClassAttributes(baseObject,
            JavaTckSupport.attributeSet(receiver, receiverMarker));
         publisher.unpublishObjectClassDirectedInteractions(childObject,
            JavaTckSupport.interactionSet(publisher, sentClass));
         publisher.unpublishObjectClassAttributes(childObject,
            JavaTckSupport.attributeSet(publisher, publisherMarker));
      } finally {
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass) {
         this.object = object;
         this.objectClass = objectClass;
      }
   }

   private static final class Directed {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle target;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;

      private Directed(InteractionClassHandle interaction, ObjectInstanceHandle target,
            ParameterHandleValueMap parameters, byte[] tag, FederateHandle producer) {
         this.interaction = interaction;
         this.target = target;
         this.parameters = parameters;
         this.tag = tag;
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new ArrayList<>();
      private final List<Directed> directed = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName())) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]));
               } else if ("receiveDirectedInteraction".equals(method.getName())) {
                  directed.add(new Directed((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1],
                     ((ParameterHandleValueMap) arguments[2]).clone(),
                     ((byte[]) arguments[3]).clone(), (FederateHandle) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK known-class recorder";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
