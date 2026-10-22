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
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/** Checks directed-interaction recipient eligibility for a derived-class target. */
final class DirectedDerivedObjectTargetEligibilityTck {
   private DirectedDerivedObjectTargetEligibilityTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName,
         String identityAttributeName, String interactionClassName,
         String integerParameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador universal = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder ownerRecorder = new Recorder();
      Recorder universalRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      RTIambassador[] ambassadors = {publisher, owner, universal, observer};
      Recorder[] recorders = {publisherRecorder, ownerRecorder, universalRecorder,
         observerRecorder};
      String federation = "java-tck-directed-derived-eligibility-" + UUID.randomUUID();
      boolean[] connected = new boolean[ambassadors.length];
      boolean[] joined = new boolean[ambassadors.length];
      boolean created = false;
      boolean ownerPublished = false, publisherSubscribed = false;
      boolean universalSubscribed = false, observerSubscribed = false;
      boolean interactionPublished = false, ownerDirected = false, universalDirected = false;
      ObjectClassHandle[] classes = new ObjectClassHandle[ambassadors.length];
      AttributeHandle[] attributes = new AttributeHandle[ambassadors.length];
      InteractionClassHandle[] interactions = new InteractionClassHandle[3];
      ParameterHandle[] parameters = new ParameterHandle[3];
      AttributeHandleSet ownerAttributes = null;
      InteractionClassHandleSet ownerInteractions = null;
      InteractionClassHandleSet universalInteractions = null;
      FederateHandle publisherHandle = null;
      FederateHandle ownerHandle = null;
      try {
         for (int i = 0; i < ambassadors.length; i++) {
            ambassadors[i].connect(recorders[i].proxy(), callbackModel);
            connected[i] = true;
         }
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisherHandle = publisher.joinFederationExecution(
            "java-tck-derived-eligibility-publisher", federation);
         joined[0] = true;
         ownerHandle = owner.joinFederationExecution(
            "java-tck-derived-eligibility-owner", federation);
         joined[1] = true;
         universal.joinFederationExecution("java-tck-derived-eligibility-universal", federation);
         joined[2] = true;
         observer.joinFederationExecution("java-tck-derived-eligibility-observer", federation);
         joined[3] = true;

         for (int i = 0; i < classes.length; i++) {
            classes[i] = ambassadors[i].getObjectClassHandle(objectClassName);
            attributes[i] = ambassadors[i].getAttributeHandle(classes[i], identityAttributeName);
            JavaTckSupport.check(classes[i] != null && attributes[i] != null,
               "derived-target class or inherited attribute lookup returned null");
            JavaTckSupport.check(objectClassName.equals(
                  ambassadors[i].getObjectClassName(classes[i]))
                  && identityAttributeName.equals(ambassadors[i].getAttributeName(
                     classes[i], attributes[i])),
               "derived-target class or inherited attribute name did not round-trip locally");
         }
         interactions[0] = publisher.getInteractionClassHandle(interactionClassName);
         interactions[1] = owner.getInteractionClassHandle(interactionClassName);
         interactions[2] = universal.getInteractionClassHandle(interactionClassName);
         for (int i = 0; i < interactions.length; i++) {
            parameters[i] = ambassadors[i].getParameterHandle(
               interactions[i], integerParameterName);
            JavaTckSupport.check(interactionClassName.equals(
                  ambassadors[i].getInteractionClassName(interactions[i]))
                  && integerParameterName.equals(ambassadors[i].getParameterName(
                     interactions[i], parameters[i])),
               "directed interaction or parameter name did not round-trip locally");
         }

         ownerAttributes = JavaTckSupport.attributeSet(owner, attributes[1]);
         owner.publishObjectClassAttributes(classes[1], ownerAttributes);
         ownerPublished = true;
         publisher.subscribeObjectClassAttributes(classes[0],
            JavaTckSupport.attributeSet(publisher, attributes[0]));
         publisherSubscribed = true;
         universal.subscribeObjectClassAttributes(classes[2],
            JavaTckSupport.attributeSet(universal, attributes[2]));
         universalSubscribed = true;
         observer.subscribeObjectClassAttributes(classes[3],
            JavaTckSupport.attributeSet(observer, attributes[3]));
         observerSubscribed = true;

         publisher.publishObjectClassDirectedInteractions(classes[0],
            JavaTckSupport.interactionSet(publisher, interactions[0]));
         interactionPublished = true;
         ownerInteractions = JavaTckSupport.interactionSet(owner, interactions[1]);
         owner.subscribeObjectClassDirectedInteractions(classes[1], ownerInteractions);
         ownerDirected = true;
         universalInteractions = JavaTckSupport.interactionSet(universal, interactions[2]);
         universal.subscribeObjectClassDirectedInteractionsUniversally(
            classes[2], universalInteractions);
         universalDirected = true;

         ObjectInstanceHandle target = owner.registerObjectInstance(classes[1]);
         JavaTckSupport.check(target != null, "derived target registration returned null");
         String targetName = owner.getObjectInstanceName(target);
         JavaTckSupport.check(targetName != null && !targetName.isEmpty(),
            "derived target registration returned no standard object name");
         for (int i : new int[] {0, 2, 3}) {
            awaitDiscovery(ambassadors[i], recorders[i], targetName, callbackModel);
            ObjectInstanceHandle localTarget = ambassadors[i].getObjectInstanceHandle(targetName);
            FederateHandle localOwner = ambassadors[i].getFederateHandle(
               owner.getFederateName(ownerHandle));
            checkDiscovery(recorders[i], targetName, localTarget, classes[i],
               localOwner, "recipient " + i);
         }

         byte[] payload = factory.getEncoderFactory().createHLAinteger32BE(0x10203040)
            .toByteArray();
         byte[] tag = new byte[] {0x44, 0x44, 0x45, 0x31};
         ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory()
            .create(1);
         values.put(parameters[0], payload);
         ObjectInstanceHandle publisherTarget = publisher.getObjectInstanceHandle(targetName);
         ObjectInstanceHandle universalTarget = universal.getObjectInstanceHandle(targetName);
         publisher.sendDirectedInteraction(interactions[0], publisherTarget, values, tag);
         awaitInteraction(owner, ownerRecorder, callbackModel);
         awaitInteraction(universal, universalRecorder, callbackModel);
         for (RTIambassador ambassador : ambassadors) pumpQuiet(ambassador, callbackModel);

         JavaTckSupport.check(ownerRecorder.interactions.size() == 1,
            "derived target owner did not receive exactly one directed interaction");
         checkDelivery(ownerRecorder.interactions.get(0), owner, interactions[1], parameters[1],
            target, payload, tag,
            owner.getFederateHandle(publisher.getFederateName(publisherHandle)), "target owner");
         JavaTckSupport.check(universalRecorder.interactions.size() == 1,
            "universal subscriber did not receive exactly one directed interaction");
         checkDelivery(universalRecorder.interactions.get(0), universal, interactions[2],
            parameters[2], universalTarget, payload, tag,
            universal.getFederateHandle(publisher.getFederateName(publisherHandle)),
            "universal subscriber");
         JavaTckSupport.check(publisherRecorder.interactions.isEmpty(),
            "publisher without a target-role subscription received a directed copy");
         JavaTckSupport.check(observerRecorder.interactions.isEmpty(),
            "attribute-only observer received a directed interaction");
         JavaTckSupport.check(publisherRecorder.unexpectedDirectedCount.get() == 0
               && ownerRecorder.unexpectedDirectedCount.get() == 0
               && universalRecorder.unexpectedDirectedCount.get() == 0
               && observerRecorder.unexpectedDirectedCount.get() == 0,
            "receive-order directed send used an unexpected callback overload");
      } finally {
         if (universalDirected && joined[2]) try {
            universal.unsubscribeObjectClassDirectedInteractions(classes[2], universalInteractions);
         } catch (Exception ignored) { }
         if (ownerDirected && joined[1]) try {
            owner.unsubscribeObjectClassDirectedInteractions(classes[1], ownerInteractions);
         } catch (Exception ignored) { }
         if (publisherSubscribed && joined[0]) unsubscribe(publisher, classes[0], attributes[0]);
         if (universalSubscribed && joined[2]) unsubscribe(universal, classes[2], attributes[2]);
         if (observerSubscribed && joined[3]) unsubscribe(observer, classes[3], attributes[3]);
         if (interactionPublished && joined[0]) try {
            publisher.unpublishObjectClassDirectedInteractions(classes[0]);
         } catch (Exception ignored) { }
         if (ownerPublished && joined[1]) try {
            owner.unpublishObjectClassAttributes(classes[1], ownerAttributes);
         } catch (Exception ignored) { }
         for (int i = ambassadors.length - 1; i >= 0; i--) {
            if (joined[i]) try {
               ambassadors[i].resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            } catch (Exception ignored) { }
         }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         for (int i = ambassadors.length - 1; i >= 0; i--) {
            if (connected[i]) try { ambassadors[i].disconnect(); }
               catch (Exception ignored) { }
         }
      }
   }

   private static void unsubscribe(RTIambassador rti, ObjectClassHandle objectClass,
         AttributeHandle attribute) {
      try { rti.unsubscribeObjectClassAttributes(objectClass,
         JavaTckSupport.attributeSet(rti, attribute)); } catch (Exception ignored) { }
   }

   private static void awaitDiscovery(RTIambassador rti, Recorder recorder,
         String targetName, CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && recorder.discoveries.stream().noneMatch(event -> targetName.equals(event.name))) {
         service(rti, callbackModel);
      }
      JavaTckSupport.check(recorder.discoveries.stream()
            .anyMatch(event -> targetName.equals(event.name)),
         "derived target discovery did not arrive before deadline; callbacks="
            + recorder.discoveries.size());
   }

   private static void awaitInteraction(RTIambassador rti, Recorder recorder,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && recorder.interactions.isEmpty()
            && recorder.unexpectedDirectedCount.get() == 0) {
         service(rti, callbackModel);
      }
      JavaTckSupport.check(!recorder.interactions.isEmpty()
            && recorder.unexpectedDirectedCount.get() == 0,
         "directed derived-target interaction did not arrive before deadline");
   }

   private static void service(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
      else Thread.yield();
   }

   private static void pumpQuiet(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(100);
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.sleep(1);
      }
   }

   private static void checkDiscovery(Recorder recorder, String targetName,
         ObjectInstanceHandle target,
         ObjectClassHandle objectClass, FederateHandle producer, String role) {
      JavaTckSupport.check(recorder.discoveries.size() == 1,
         role + " did not receive exactly one discovery");
      Discovery event = recorder.discoveries.get(0);
      JavaTckSupport.check(targetName.equals(event.name)
            && target.equals(event.object) && objectClass.equals(event.objectClass)
            && producer.equals(event.producer),
         role + " discovery returned incorrect target, derived class, or producer");
   }

   private static void checkDelivery(Interaction actual, RTIambassador inspector,
         InteractionClassHandle interaction, ParameterHandle parameter,
         ObjectInstanceHandle target, byte[] payload, byte[] tag, FederateHandle producer,
         String role) throws Exception {
      JavaTckSupport.check(interaction.equals(actual.interaction)
            && target.equals(actual.target) && actual.parameters.size() == 1
            && Arrays.equals(payload, actual.parameters.get(parameter)),
         role + " received the wrong interaction, target, or parameter value");
      JavaTckSupport.check(Arrays.equals(tag, actual.tag) && producer.equals(actual.producer),
         role + " received the wrong tag or producing federate");
      JavaTckSupport.check(actual.transportation != null
            && !inspector.getTransportationTypeName(actual.transportation).isEmpty(),
         role + " received an unresolvable transportation type");
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;
      private final FederateHandle producer;
      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String name, FederateHandle producer) {
         this.object = object; this.objectClass = objectClass;
         this.name = name; this.producer = producer;
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle target;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private Interaction(InteractionClassHandle interaction, ObjectInstanceHandle target,
            ParameterHandleValueMap parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer) {
         this.interaction = interaction; this.target = target; this.parameters = parameters;
         this.tag = tag; this.transportation = transportation; this.producer = producer;
      }
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
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2],
                     (FederateHandle) arguments[3]));
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1],
                     ((ParameterHandleValueMap) arguments[2]).clone(),
                     ((byte[]) arguments[3]).clone(), (TransportationTypeHandle) arguments[4],
                     (FederateHandle) arguments[5]));
               } else if ("receiveDirectedInteraction".equals(name)) {
                  unexpectedDirectedCount.incrementAndGet();
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK derived eligibility callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
