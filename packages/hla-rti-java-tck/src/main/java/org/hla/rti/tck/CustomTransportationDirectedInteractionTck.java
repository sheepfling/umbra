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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks custom-transport delivery to targeted and universal directed subscribers. */
final class CustomTransportationDirectedInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private CustomTransportationDirectedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String interactionName, String parameterName,
         String transportationName, CallbackModel callbackModel) throws Exception {
      RTIambassador producer = factory.getRtiAmbassador();
      RTIambassador targeted = factory.getRtiAmbassador();
      RTIambassador universal = factory.getRtiAmbassador();
      Recorder producerCallbacks = new Recorder();
      Recorder targetedCallbacks = new Recorder();
      Recorder universalCallbacks = new Recorder();
      String federation = "java-tck-custom-transport-directed-" + UUID.randomUUID();
      boolean[] connected = new boolean[3];
      boolean created = false;
      boolean[] joined = new boolean[3];
      boolean producerAttributesSubscribed = false, universalAttributesSubscribed = false;
      boolean targetedAttributesPublished = false, targetedAttributesSubscribed = false;
      boolean published = false, targetedSubscribed = false, universalSubscribed = false;
      ObjectClassHandle producerClass = null, targetedClass = null, universalClass = null;
      AttributeHandleSet producerAttributes = null, targetedAttributes = null;
      AttributeHandleSet universalAttributes = null;
      InteractionClassHandleSet producerDirected = null, targetedDirected = null;
      InteractionClassHandleSet universalDirected = null;
      try {
         producer.connect(producerCallbacks.proxy(), callbackModel);
         connected[0] = true;
         targeted.connect(targetedCallbacks.proxy(), callbackModel);
         connected[1] = true;
         universal.connect(universalCallbacks.proxy(), callbackModel);
         connected[2] = true;
         producer.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producerHandle = producer.joinFederationExecution(
            "java-tck-custom-transport-directed-producer", federation);
         joined[0] = true;
         targeted.joinFederationExecution("java-tck-custom-transport-directed-targeted",
            federation);
         joined[1] = true;
         universal.joinFederationExecution("java-tck-custom-transport-directed-universal",
            federation);
         joined[2] = true;

         producerClass = producer.getObjectClassHandle(objectClassName);
         targetedClass = targeted.getObjectClassHandle(objectClassName);
         universalClass = universal.getObjectClassHandle(objectClassName);
         AttributeHandle producerAttribute = producer.getAttributeHandle(producerClass,
            attributeName);
         AttributeHandle targetedAttribute = targeted.getAttributeHandle(targetedClass,
            attributeName);
         AttributeHandle universalAttribute = universal.getAttributeHandle(universalClass,
            attributeName);
         producerAttributes = JavaTckSupport.attributeSet(producer, producerAttribute);
         targetedAttributes = JavaTckSupport.attributeSet(targeted, targetedAttribute);
         universalAttributes = JavaTckSupport.attributeSet(universal, universalAttribute);

         InteractionClassHandle producerInteraction =
            producer.getInteractionClassHandle(interactionName);
         InteractionClassHandle targetedInteraction =
            targeted.getInteractionClassHandle(interactionName);
         InteractionClassHandle universalInteraction =
            universal.getInteractionClassHandle(interactionName);
         ParameterHandle producerParameter = producer.getParameterHandle(producerInteraction,
            parameterName);
         ParameterHandle targetedParameter = targeted.getParameterHandle(targetedInteraction,
            parameterName);
         ParameterHandle universalParameter = universal.getParameterHandle(universalInteraction,
            parameterName);
         TransportationTypeHandle producerTransportation =
            producer.getTransportationTypeHandle(transportationName);
         TransportationTypeHandle targetedTransportation =
            targeted.getTransportationTypeHandle(transportationName);
         TransportationTypeHandle universalTransportation =
            universal.getTransportationTypeHandle(transportationName);
         JavaTckSupport.check(objectClassName.equals(producer.getObjectClassName(producerClass))
               && objectClassName.equals(targeted.getObjectClassName(targetedClass))
               && objectClassName.equals(universal.getObjectClassName(universalClass))
               && attributeName.equals(producer.getAttributeName(producerClass, producerAttribute))
               && attributeName.equals(targeted.getAttributeName(targetedClass, targetedAttribute))
               && attributeName.equals(universal.getAttributeName(universalClass,
                  universalAttribute))
               && interactionName.equals(
               producer.getInteractionClassName(producerInteraction))
               && interactionName.equals(targeted.getInteractionClassName(targetedInteraction))
               && interactionName.equals(universal.getInteractionClassName(universalInteraction))
               && parameterName.equals(producer.getParameterName(producerInteraction,
                  producerParameter))
               && parameterName.equals(targeted.getParameterName(targetedInteraction,
                  targetedParameter))
               && parameterName.equals(universal.getParameterName(universalInteraction,
                  universalParameter))
               && transportationName.equals(producer.getTransportationTypeName(
                  producerTransportation))
               && transportationName.equals(targeted.getTransportationTypeName(
                  targetedTransportation))
               && transportationName.equals(universal.getTransportationTypeName(
                  universalTransportation)),
            "adapter FOM directed interaction, parameter, or transportation name did not round-trip");

         producer.subscribeObjectClassAttributes(producerClass, producerAttributes);
         producerAttributesSubscribed = true;
         targeted.publishObjectClassAttributes(targetedClass, targetedAttributes);
         targetedAttributesPublished = true;
         targeted.subscribeObjectClassAttributes(targetedClass, targetedAttributes);
         targetedAttributesSubscribed = true;
         universal.subscribeObjectClassAttributes(universalClass, universalAttributes);
         universalAttributesSubscribed = true;
         producerDirected = JavaTckSupport.interactionSet(producer, producerInteraction);
         targetedDirected = JavaTckSupport.interactionSet(targeted, targetedInteraction);
         universalDirected = JavaTckSupport.interactionSet(universal, universalInteraction);
         producer.publishObjectClassDirectedInteractions(producerClass, producerDirected);
         published = true;
         targeted.subscribeObjectClassDirectedInteractions(targetedClass, targetedDirected);
         targetedSubscribed = true;
         universal.subscribeObjectClassDirectedInteractionsUniversally(universalClass,
            universalDirected);
         universalSubscribed = true;

         ObjectInstanceHandle target = targeted.registerObjectInstance(targetedClass);
         String targetName = targeted.getObjectInstanceName(target);
         JavaTckSupport.check(targetName != null && !targetName.isEmpty()
               && target.equals(targeted.getObjectInstanceHandle(targetName)),
            "targeted object registration did not preserve object-name lookup");
         awaitDiscovery(producer, universal, producerCallbacks, universalCallbacks, target,
            callbackModel);
         byte[] payload = new byte[] {0x43, 0x55, 0x53, 0x54};
         byte[] tag = new byte[] {0x44, 0x49, 0x52, 0x2d, 0x43};
         ParameterHandleValueMap parameters =
            producer.getParameterHandleValueMapFactory().create(1);
         parameters.put(producerParameter, payload.clone());
         producer.sendDirectedInteraction(producerInteraction, target, parameters, tag.clone());
         awaitDelivery(targeted, callbackModel, targetedCallbacks);
         awaitDelivery(universal, callbackModel, universalCallbacks);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(producer);
         JavaTckSupport.check(targetedCallbacks.directed.size() == 1
               && universalCallbacks.directed.size() == 1
               && producerCallbacks.directed.isEmpty(),
            "custom-transport directed interaction reached an unexpected recipient");
         assertDelivery(targetedCallbacks.directed.get(0), targetedInteraction, target,
            targetedParameter, payload, tag, producerHandle, targeted, transportationName,
            "targeted callback");
         assertDelivery(universalCallbacks.directed.get(0), universalInteraction, target,
            universalParameter, payload, tag, producerHandle, universal, transportationName,
            "universal callback");

         universal.unsubscribeObjectClassDirectedInteractions(universalClass, universalDirected);
         universalSubscribed = false;
         targeted.unsubscribeObjectClassDirectedInteractions(targetedClass, targetedDirected);
         targetedSubscribed = false;
         producer.unpublishObjectClassDirectedInteractions(producerClass, producerDirected);
         published = false;
         universal.unsubscribeObjectClassAttributes(universalClass, universalAttributes);
         universalAttributesSubscribed = false;
         targeted.unsubscribeObjectClassAttributes(targetedClass, targetedAttributes);
         targetedAttributesSubscribed = false;
         targeted.unpublishObjectClassAttributes(targetedClass, targetedAttributes);
         targetedAttributesPublished = false;
         producer.unsubscribeObjectClassAttributes(producerClass, producerAttributes);
         producerAttributesSubscribed = false;
      } finally {
         if (joined[2] && universalSubscribed) {
            try { universal.unsubscribeObjectClassDirectedInteractions(universalClass,
               universalDirected); } catch (Exception ignored) { }
         }
         if (joined[1] && targetedSubscribed) {
            try { targeted.unsubscribeObjectClassDirectedInteractions(targetedClass,
               targetedDirected); } catch (Exception ignored) { }
         }
         if (joined[0] && published) {
            try { producer.unpublishObjectClassDirectedInteractions(producerClass,
               producerDirected); } catch (Exception ignored) { }
         }
         if (joined[2] && universalAttributesSubscribed) {
            try { universal.unsubscribeObjectClassAttributes(universalClass,
               universalAttributes); } catch (Exception ignored) { }
         }
         if (joined[1] && targetedAttributesSubscribed) {
            try { targeted.unsubscribeObjectClassAttributes(targetedClass,
               targetedAttributes); } catch (Exception ignored) { }
         }
         if (joined[1] && targetedAttributesPublished) {
            try { targeted.unpublishObjectClassAttributes(targetedClass,
               targetedAttributes); } catch (Exception ignored) { }
         }
         if (joined[0] && producerAttributesSubscribed) {
            try { producer.unsubscribeObjectClassAttributes(producerClass,
               producerAttributes); } catch (Exception ignored) { }
         }
         resign(producer, joined[0]);
         resign(targeted, joined[1]);
         resign(universal, joined[2]);
         if (created) {
            try { producer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         disconnect(universal, connected[2]);
         disconnect(targeted, connected[1]);
         disconnect(producer, connected[0]);
      }
   }

   private static void awaitDiscovery(RTIambassador producer, RTIambassador universal,
         Recorder producerCallbacks, Recorder universalCallbacks, ObjectInstanceHandle target,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline
            && (!producerCallbacks.discoveries.contains(target)
               || !universalCallbacks.discoveries.contains(target))) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(producer);
            JavaTckSupport.drain(universal);
         }
         Thread.yield();
      }
      JavaTckSupport.check(producerCallbacks.discoveries.contains(target)
            && universalCallbacks.discoveries.contains(target),
         "targeted registration was not discovered by the producer and universal member");
   }

   private static void awaitDelivery(RTIambassador ambassador, CallbackModel callbackModel,
         Recorder recorder) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && recorder.directed.isEmpty()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(!recorder.directed.isEmpty(),
         "custom-transport directed interaction callback was not delivered");
   }

   private static void assertDelivery(Directed received, InteractionClassHandle interaction,
         ObjectInstanceHandle target, ParameterHandle parameter, byte[] payload, byte[] tag,
         FederateHandle producer, RTIambassador inspector, String transportationName,
         String description) throws Exception {
      JavaTckSupport.check(interaction.equals(received.interaction)
            && target.equals(received.target)
            && received.parameters.size() == 1
            && received.parameters.containsKey(parameter)
            && Arrays.equals(payload, received.parameters.get(parameter))
            && Arrays.equals(tag, received.tag)
            && producer.equals(received.producer)
            && received.transportation != null
            && transportationName.equals(inspector.getTransportationTypeName(
               received.transportation)),
         description + " changed target, value, tag, producer, or transportation metadata");
   }

   private static void resign(RTIambassador ambassador, boolean joined) {
      if (joined) {
         try { ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
         catch (Exception ignored) { }
      }
   }

   private static void disconnect(RTIambassador ambassador, boolean connected) {
      if (connected) { try { ambassador.disconnect(); } catch (Exception ignored) { } }
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
      private final List<Directed> directed = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();

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
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK custom directed callbacks";
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
