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

import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** P1 ordinary directed interaction FIFO fan-out to multiple universal subscribers. */
final class DirectedInteractionMultiRecipientFifoTck {
   private DirectedInteractionMultiRecipientFifoTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String interactionName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      String federation = "java-tck-directed-interaction-multi-recipient-fifo-"
         + UUID.randomUUID();
      boolean publisherConnected = false, firstConnected = false, secondConnected = false;
      boolean created = false, publisherJoined = false, firstJoined = false, secondJoined = false;
      boolean objectPublished = false, firstObjectSubscribed = false;
      boolean secondObjectSubscribed = false, directedPublished = false;
      boolean firstDirectedSubscribed = false, secondDirectedSubscribed = false;
      ObjectClassHandle publisherClass = null, firstClass = null, secondClass = null;
      AttributeHandleSet publisherAttributes = null, firstAttributes = null, secondAttributes = null;
      InteractionClassHandle publisherInteraction = null, firstInteraction = null;
      InteractionClassHandle secondInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         first.connect(firstRecorder.proxy(), callbackModel);
         firstConnected = true;
         second.connect(secondRecorder.proxy(), callbackModel);
         secondConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-multi-recipient-publisher", federation);
         publisherJoined = true;
         first.joinFederationExecution("java-tck-directed-multi-recipient-first", federation);
         firstJoined = true;
         second.joinFederationExecution("java-tck-directed-multi-recipient-second", federation);
         secondJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         firstClass = first.getObjectClassHandle(objectClassName);
         secondClass = second.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         secondAttributes = JavaTckSupport.attributeSet(second, secondAttribute);
         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         firstInteraction = first.getInteractionClassHandle(interactionName);
         secondInteraction = second.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle firstParameter = first.getParameterHandle(firstInteraction, parameterName);
         ParameterHandle secondParameter = second.getParameterHandle(secondInteraction, parameterName);

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         objectPublished = true;
         first.subscribeObjectClassAttributes(firstClass, firstAttributes);
         firstObjectSubscribed = true;
         second.subscribeObjectClassAttributes(secondClass, secondAttributes);
         secondObjectSubscribed = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         directedPublished = true;
         first.subscribeObjectClassDirectedInteractionsUniversally(firstClass,
            JavaTckSupport.interactionSet(first, firstInteraction));
         firstDirectedSubscribed = true;
         second.subscribeObjectClassDirectedInteractionsUniversally(secondClass,
            JavaTckSupport.interactionSet(second, secondInteraction));
         secondDirectedSubscribed = true;

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         awaitDiscovery(first, second, firstRecorder, secondRecorder, target);
         JavaTckSupport.check(firstRecorder.discoveries.contains(target)
               && secondRecorder.discoveries.contains(target),
            "both universal subscribers must discover the directed-interaction target");
         publisherRecorder.directed.clear();
         firstRecorder.directed.clear();
         secondRecorder.directed.clear();

         byte[] firstValue = new byte[] {0x46, 0x49, 0x52, 0x53, 0x54};
         byte[] firstTag = new byte[] {0x31};
         byte[] secondValue = new byte[] {0x53, 0x45, 0x43, 0x4f, 0x4e, 0x44};
         byte[] secondTag = new byte[] {0x32};
         send(publisher, publisherInteraction, target, publisherParameter, firstValue, firstTag);
         send(publisher, publisherInteraction, target, publisherParameter, secondValue, secondTag);

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(firstRecorder.directed.isEmpty()
                  && secondRecorder.directed.isEmpty(),
               "evoked directed interactions arrived before callback servicing");
         }
         awaitBoth(first, second, firstRecorder, secondRecorder);
         JavaTckSupport.check(firstRecorder.directed.size() == 2
               && secondRecorder.directed.size() == 2,
            "each universal subscriber must receive exactly two directed interactions");
         assertDelivery(firstRecorder.directed, firstInteraction, target, firstParameter,
            firstValue, firstTag, secondValue, secondTag, producer, first, "first subscriber");
         assertDelivery(secondRecorder.directed, secondInteraction, target, secondParameter,
            firstValue, firstTag, secondValue, secondTag, producer, second, "second subscriber");
         JavaTckSupport.check(publisherRecorder.directed.isEmpty(),
            "directed interactions were delivered back to their sender");

         first.unsubscribeObjectClassDirectedInteractions(firstClass);
         firstDirectedSubscribed = false;
         second.unsubscribeObjectClassDirectedInteractions(secondClass);
         secondDirectedSubscribed = false;
         publisher.unpublishObjectClassDirectedInteractions(publisherClass);
         directedPublished = false;
         first.unsubscribeObjectClass(firstClass);
         firstObjectSubscribed = false;
         second.unsubscribeObjectClass(secondClass);
         secondObjectSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         objectPublished = false;
      } finally {
         if (secondDirectedSubscribed && secondJoined) {
            try { second.unsubscribeObjectClassDirectedInteractions(secondClass); }
            catch (Exception ignored) { }
         }
         if (firstDirectedSubscribed && firstJoined) {
            try { first.unsubscribeObjectClassDirectedInteractions(firstClass); }
            catch (Exception ignored) { }
         }
         if (directedPublished && publisherJoined) {
            try { publisher.unpublishObjectClassDirectedInteractions(publisherClass); }
            catch (Exception ignored) { }
         }
         if (secondObjectSubscribed && secondJoined) {
            try { second.unsubscribeObjectClass(secondClass); }
            catch (Exception ignored) { }
         }
         if (firstObjectSubscribed && firstJoined) {
            try { first.unsubscribeObjectClass(firstClass); }
            catch (Exception ignored) { }
         }
         if (objectPublished && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (secondJoined) {
            try { second.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (firstJoined) {
            try { first.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (secondConnected) {
            try { second.disconnect(); } catch (Exception ignored) { }
         }
         if (firstConnected) {
            try { first.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ObjectInstanceHandle target, ParameterHandle parameter, byte[] value, byte[] tag)
         throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value);
      publisher.sendDirectedInteraction(interaction, target, values, tag);
   }

   private static void awaitDiscovery(RTIambassador first, RTIambassador second,
         Recorder firstRecorder, Recorder secondRecorder, ObjectInstanceHandle target)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (!firstRecorder.discoveries.contains(target)
               || !secondRecorder.discoveries.contains(target))) {
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         Thread.yield();
      }
      JavaTckSupport.check(firstRecorder.discoveries.contains(target)
            && secondRecorder.discoveries.contains(target),
         "both subscribers did not discover the directed target before deadline");
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         Recorder firstRecorder, Recorder secondRecorder) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (firstRecorder.directed.size() < 2 || secondRecorder.directed.size() < 2)) {
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         Thread.yield();
      }
      JavaTckSupport.check(firstRecorder.directed.size() >= 2
            && secondRecorder.directed.size() >= 2,
         "both directed multi-recipient streams did not arrive before deadline");
   }

   private static void assertDelivery(List<Directed> received,
         InteractionClassHandle interaction, ObjectInstanceHandle target,
         ParameterHandle parameter, byte[] firstValue, byte[] firstTag,
         byte[] secondValue, byte[] secondTag, FederateHandle producer,
         RTIambassador inspector, String description) throws Exception {
      assertRecord(received.get(0), interaction, target, parameter, firstValue, firstTag,
         producer, inspector, description + " first receive-order callback");
      assertRecord(received.get(1), interaction, target, parameter, secondValue, secondTag,
         producer, inspector, description + " second receive-order callback");
   }

   private static void assertRecord(Directed received, InteractionClassHandle interaction,
         ObjectInstanceHandle target, ParameterHandle parameter, byte[] value, byte[] tag,
         FederateHandle producer, RTIambassador inspector, String description)
         throws Exception {
      JavaTckSupport.check(interaction.equals(received.interaction)
            && target.equals(received.target), description + " carried the wrong class or target");
      JavaTckSupport.check(received.parameters.size() == 1
            && received.parameters.containsKey(parameter)
            && Arrays.equals(value, received.parameters.get(parameter)),
         description + " carried the wrong parameter value");
      JavaTckSupport.check(Arrays.equals(tag, received.tag),
         description + " did not preserve the user tag");
      JavaTckSupport.check(producer.equals(received.producer),
         description + " carried the wrong producing federate");
      JavaTckSupport.check(received.transportation != null
            && inspector.getTransportationTypeName(received.transportation) != null
            && !inspector.getTransportationTypeName(received.transportation).isEmpty(),
         description + " carried an unknown transportation type");
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
               if ("receiveDirectedInteraction".equals(name)) {
                  ParameterHandleValueMap parameters =
                     ((ParameterHandleValueMap) arguments[2]).clone();
                  byte[] tag = ((byte[]) arguments[3]).clone();
                  directed.add(new Directed((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1], parameters, tag,
                     (TransportationTypeHandle) arguments[4], (FederateHandle) arguments[5]));
               } else if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed FIFO callbacks";
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
