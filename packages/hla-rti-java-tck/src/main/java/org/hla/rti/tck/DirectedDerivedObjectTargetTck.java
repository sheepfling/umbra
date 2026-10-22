package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
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
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/** Delivers an ordinary directed interaction to a discovered derived-class object. */
final class DirectedDerivedObjectTargetTck {
   private DirectedDerivedObjectTargetTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName,
         String identityAttributeName, String interactionClassName,
         String integerParameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador targeted = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder publisherRec = new Recorder();
      Recorder targetedRec = new Recorder();
      Recorder observerRec = new Recorder();
      String federation = "java-tck-directed-derived-target-" + UUID.randomUUID();
      boolean publisherConnected = false, targetedConnected = false, observerConnected = false;
      boolean created = false, publisherJoined = false, targetedJoined = false, observerJoined = false;
      boolean attributesPublished = false, targetedAttributesSubscribed = false;
      boolean observerAttributesSubscribed = false, directedPublished = false;
      boolean directedSubscribed = false;
      try {
         publisher.connect(publisherRec.proxy(), callbackModel); publisherConnected = true;
         targeted.connect(targetedRec.proxy(), callbackModel); targetedConnected = true;
         observer.connect(observerRec.proxy(), callbackModel); observerConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-derived-publisher", federation); publisherJoined = true;
         targeted.joinFederationExecution("java-tck-directed-derived-subscriber", federation);
         targetedJoined = true;
         observer.joinFederationExecution("java-tck-directed-observer", federation);
         observerJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle targetedClass = targeted.getObjectClassHandle(objectClassName);
         ObjectClassHandle observerClass = observer.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(targeted.getObjectClassName(targetedClass))
               && objectClassName.equals(observer.getObjectClassName(observerClass)),
            "derived object class did not round-trip locally");
         AttributeHandle publisherIdentity = publisher.getAttributeHandle(
            publisherClass, identityAttributeName);
         AttributeHandle targetedIdentity = targeted.getAttributeHandle(
            targetedClass, identityAttributeName);
         AttributeHandle observerIdentity = observer.getAttributeHandle(
            observerClass, identityAttributeName);
         JavaTckSupport.check(identityAttributeName.equals(publisher.getAttributeName(
                  publisherClass, publisherIdentity))
               && identityAttributeName.equals(targeted.getAttributeName(
                  targetedClass, targetedIdentity))
               && identityAttributeName.equals(observer.getAttributeName(
                  observerClass, observerIdentity)),
            "inherited identity attribute did not round-trip locally");
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle targetedInteraction =
            targeted.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(interactionClassName.equals(
                  publisher.getInteractionClassName(publisherInteraction))
               && interactionClassName.equals(
                  targeted.getInteractionClassName(targetedInteraction)),
            "directed interaction class did not round-trip locally");
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, integerParameterName);
         ParameterHandle targetedParameter = targeted.getParameterHandle(
            targetedInteraction, integerParameterName);
         JavaTckSupport.check(integerParameterName.equals(publisher.getParameterName(
                  publisherInteraction, publisherParameter))
               && integerParameterName.equals(targeted.getParameterName(
                  targetedInteraction, targetedParameter)),
            "directed interaction parameter did not round-trip locally");

         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherIdentity);
         AttributeHandleSet targetedAttributes =
            JavaTckSupport.attributeSet(targeted, targetedIdentity);
         AttributeHandleSet observerAttributes =
            JavaTckSupport.attributeSet(observer, observerIdentity);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         attributesPublished = true;
         targeted.subscribeObjectClassAttributes(targetedClass, targetedAttributes);
         targetedAttributesSubscribed = true;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);
         observerAttributesSubscribed = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         directedPublished = true;
         targeted.subscribeObjectClassDirectedInteractionsUniversally(targetedClass,
            JavaTckSupport.interactionSet(targeted, targetedInteraction));
         directedSubscribed = true;

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         String targetName = publisher.getObjectInstanceName(target);
         JavaTckSupport.check(targetName != null && !targetName.isEmpty(),
            "derived target registration returned no standard object name");
         awaitDiscovery(targeted, targetedRec, targetName, callbackModel);
         awaitDiscovery(observer, observerRec, targetName, callbackModel);
         ObjectInstanceHandle targetedTarget = targeted.getObjectInstanceHandle(targetName);
         ObjectInstanceHandle observerTarget = observer.getObjectInstanceHandle(targetName);
         String producerName = publisher.getFederateName(producer);
         FederateHandle targetedProducer = targeted.getFederateHandle(producerName);
         FederateHandle observerProducer = observer.getFederateHandle(producerName);
         checkDiscovery(targetedRec, targetName, targetedTarget, targetedClass,
            targetedProducer, "target subscriber");
         checkDiscovery(observerRec, targetName, observerTarget, observerClass,
            observerProducer, "non-directed observer");

         HLAinteger32BE encodedValue = factory.getEncoderFactory()
            .createHLAinteger32BE(0x2468ace0);
         byte[] payload = encodedValue.toByteArray();
         byte[] tag = new byte[] {0x44, 0x44, 0x4f, 0x31};
         ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory()
            .create(1);
         parameters.put(publisherParameter, payload);
         publisher.sendDirectedInteraction(publisherInteraction, target, parameters, tag);
         awaitInteraction(targeted, targetedRec, callbackModel);
         pumpQuiet(publisher, callbackModel);
         pumpQuiet(targeted, callbackModel);
         pumpQuiet(observer, callbackModel);
         JavaTckSupport.check(targetedRec.interactions.size() == 1,
            "directed subscriber did not receive exactly one interaction");
         checkDelivery(targetedRec, targeted, targetedInteraction, targetedParameter,
            targetedTarget, payload, tag, targetedProducer);
         JavaTckSupport.check(observerRec.interactions.isEmpty(),
            "observer without a directed subscription received the interaction");
         JavaTckSupport.check(publisherRec.interactions.isEmpty(),
            "publisher without a directed subscription received a loopback interaction");
         JavaTckSupport.check(publisherRec.unexpectedDirectedCount.get() == 0
               && targetedRec.unexpectedDirectedCount.get() == 0
               && observerRec.unexpectedDirectedCount.get() == 0,
            "receive-order directed send used an unexpected callback overload");
      } finally {
         if (directedSubscribed && targetedJoined) try {
            targeted.unsubscribeObjectClassDirectedInteractions(
               targeted.getObjectClassHandle(objectClassName));
         } catch (Exception ignored) { }
         if (targetedAttributesSubscribed && targetedJoined)
            unsubscribe(targeted, objectClassName, identityAttributeName);
         if (observerAttributesSubscribed && observerJoined)
            unsubscribe(observer, objectClassName, identityAttributeName);
         if (directedPublished && publisherJoined) try {
            publisher.unpublishObjectClassDirectedInteractions(
               publisher.getObjectClassHandle(objectClassName));
         } catch (Exception ignored) { }
         if (attributesPublished && publisherJoined) try {
            ObjectClassHandle cls = publisher.getObjectClassHandle(objectClassName);
            publisher.unpublishObjectClassAttributes(cls,
               JavaTckSupport.attributeSet(publisher,
                  publisher.getAttributeHandle(cls, identityAttributeName)));
         } catch (Exception ignored) { }
         if (targetedJoined) try { targeted.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (observerJoined) try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (observerConnected) try { observer.disconnect(); } catch (Exception ignored) { }
         if (targetedConnected) try { targeted.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void awaitDiscovery(RTIambassador rti, Recorder recorder,
         String targetName, CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(10);
      while (System.nanoTime() < deadline && recorder.discoveries.stream()
            .noneMatch(event -> targetName.equals(event.name))) service(rti, callbackModel);
      JavaTckSupport.check(recorder.discoveries.stream()
            .anyMatch(event -> targetName.equals(event.name)),
         "derived target discovery did not arrive before deadline");
   }

   private static void awaitInteraction(RTIambassador rti, Recorder recorder,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(10);
      while (System.nanoTime() < deadline && recorder.interactions.isEmpty()
            && recorder.unexpectedDirectedCount.get() == 0) service(rti, callbackModel);
      JavaTckSupport.check(!recorder.interactions.isEmpty()
            && recorder.unexpectedDirectedCount.get() == 0,
         "directed interaction did not arrive before deadline");
   }

   private static void service(RTIambassador rti, CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) rti.evokeCallback(0.01);
      else Thread.yield();
   }

   private static void pumpQuiet(RTIambassador rti, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(100);
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) rti.evokeCallback(0.01);
         else Thread.sleep(1);
      }
   }

   private static void checkDiscovery(Recorder recorder, String name, ObjectInstanceHandle object,
         ObjectClassHandle objectClass, FederateHandle producer, String role) {
      JavaTckSupport.check(recorder.discoveries.size() == 1,
         role + " did not receive exactly one object discovery");
      Discovery discovery = recorder.discoveries.get(0);
      JavaTckSupport.check(name.equals(discovery.name) && object.equals(discovery.object)
            && objectClass.equals(discovery.objectClass) && producer.equals(discovery.producer),
         role + " discovery returned incorrect object, class, or producer metadata");
   }

   private static void checkDelivery(Recorder recorder, RTIambassador inspector,
         InteractionClassHandle interaction, ParameterHandle parameter,
         ObjectInstanceHandle target, byte[] payload, byte[] tag, FederateHandle producer)
         throws Exception {
      Interaction actual = recorder.interactions.get(0);
      JavaTckSupport.check(interaction.equals(actual.interactionClass)
            && target.equals(actual.object) && actual.parameters != null
            && actual.parameters.size() == 1
            && Arrays.equals(payload, actual.parameters.get(parameter)),
         "directed callback returned the wrong interaction, target, or payload");
      JavaTckSupport.check(Arrays.equals(tag, actual.tag) && producer.equals(actual.producer),
         "directed callback changed the tag or producing federate");
      JavaTckSupport.check(actual.transportation != null
            && !inspector.getTransportationTypeName(actual.transportation).isEmpty(),
         "directed callback returned an unresolvable transportation type");
   }

   private static void unsubscribe(RTIambassador rti, String className, String attributeName) {
      try {
         ObjectClassHandle cls = rti.getObjectClassHandle(className);
         AttributeHandle attribute = rti.getAttributeHandle(cls, attributeName);
         rti.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(rti, attribute));
      } catch (Exception ignored) { }
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private String name;
      private FederateHandle producer;
   }

   private static final class Interaction {
      private InteractionClassHandle interactionClass;
      private ObjectInstanceHandle object;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final AtomicInteger unexpectedDirectedCount = new AtomicInteger();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 4) {
                  Discovery event = new Discovery();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.objectClass = (ObjectClassHandle) arguments[1];
                  event.name = (String) arguments[2];
                  event.producer = (FederateHandle) arguments[3];
                  discoveries.add(event);
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interactionClass = (InteractionClassHandle) arguments[0];
                  event.object = (ObjectInstanceHandle) arguments[1];
                  event.parameters = ((ParameterHandleValueMap) arguments[2]).clone();
                  event.tag = ((byte[]) arguments[3]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[4];
                  event.producer = (FederateHandle) arguments[5];
                  interactions.add(event);
               } else if ("receiveDirectedInteraction".equals(name)) {
                  unexpectedDirectedCount.incrementAndGet();
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed-derived callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
