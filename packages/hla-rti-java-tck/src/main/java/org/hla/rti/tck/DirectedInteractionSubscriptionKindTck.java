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

/** Distinguish by-ownership and universal directed-interaction subscriptions. */
final class DirectedInteractionSubscriptionKindTck {
   private DirectedInteractionSubscriptionKindTck() {
   }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String interactionName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador secondOwner = factory.getRtiAmbassador();
      RTIambassador selector = factory.getRtiAmbassador();
      Recorder senderRecorder = new Recorder();
      Recorder ownerRecorder = new Recorder();
      Recorder secondOwnerRecorder = new Recorder();
      Recorder selectorRecorder = new Recorder();
      String federation = "java-tck-directed-subscription-kind-" + UUID.randomUUID();
      boolean[] connected = new boolean[4];
      boolean created = false;
      boolean[] joined = new boolean[4];
      boolean ownerPublished = false, secondOwnerPublished = false;
      boolean senderSubscribed = false, selectorSubscribed = false;
      boolean interactionPublished = false, ownerDirectedSubscribed = false;
      boolean secondOwnerDirectedSubscribed = false, selectorDirectedSubscribed = false;
      ObjectClassHandle senderClass = null, ownerClass = null;
      ObjectClassHandle secondOwnerClass = null, selectorClass = null;
      AttributeHandleSet senderAttributes = null, ownerAttributes = null;
      AttributeHandleSet secondOwnerAttributes = null, selectorAttributes = null;
      InteractionClassHandle senderInteraction = null, ownerInteraction = null;
      InteractionClassHandle secondOwnerInteraction = null, selectorInteraction = null;
      InteractionClassHandleSet senderDirected = null, ownerDirected = null;
      InteractionClassHandleSet secondOwnerDirected = null, selectorDirected = null;
      try {
         sender.connect(senderRecorder.proxy(), callbackModel);
         connected[0] = true;
         owner.connect(ownerRecorder.proxy(), callbackModel);
         connected[1] = true;
         secondOwner.connect(secondOwnerRecorder.proxy(), callbackModel);
         connected[2] = true;
         selector.connect(selectorRecorder.proxy(), callbackModel);
         connected[3] = true;
         sender.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-subscription-kind-sender", federation);
         joined[0] = true;
         owner.joinFederationExecution("java-tck-subscription-kind-owner", federation);
         joined[1] = true;
         secondOwner.joinFederationExecution("java-tck-subscription-kind-second-owner",
            federation);
         joined[2] = true;
         selector.joinFederationExecution("java-tck-subscription-kind-selector", federation);
         joined[3] = true;

         senderClass = sender.getObjectClassHandle(objectClassName);
         ownerClass = owner.getObjectClassHandle(objectClassName);
         secondOwnerClass = secondOwner.getObjectClassHandle(objectClassName);
         selectorClass = selector.getObjectClassHandle(objectClassName);
         AttributeHandle senderAttribute = sender.getAttributeHandle(senderClass, attributeName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle secondOwnerAttribute = secondOwner.getAttributeHandle(
            secondOwnerClass, attributeName);
         AttributeHandle selectorAttribute = selector.getAttributeHandle(selectorClass,
            attributeName);
         senderAttributes = JavaTckSupport.attributeSet(sender, senderAttribute);
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         secondOwnerAttributes = JavaTckSupport.attributeSet(secondOwner, secondOwnerAttribute);
         selectorAttributes = JavaTckSupport.attributeSet(selector, selectorAttribute);
         senderInteraction = sender.getInteractionClassHandle(interactionName);
         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         secondOwnerInteraction = secondOwner.getInteractionClassHandle(interactionName);
         selectorInteraction = selector.getInteractionClassHandle(interactionName);
         ParameterHandle senderParameter = sender.getParameterHandle(senderInteraction,
            parameterName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction,
            parameterName);
         ParameterHandle secondOwnerParameter = secondOwner.getParameterHandle(
            secondOwnerInteraction, parameterName);
         ParameterHandle selectorParameter = selector.getParameterHandle(selectorInteraction,
            parameterName);

         sender.subscribeObjectClassAttributes(senderClass, senderAttributes);
         senderSubscribed = true;
         selector.subscribeObjectClassAttributes(selectorClass, selectorAttributes);
         selectorSubscribed = true;
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         secondOwner.publishObjectClassAttributes(secondOwnerClass, secondOwnerAttributes);
         secondOwnerPublished = true;

         senderDirected = JavaTckSupport.interactionSet(sender, senderInteraction);
         ownerDirected = JavaTckSupport.interactionSet(owner, ownerInteraction);
         secondOwnerDirected = JavaTckSupport.interactionSet(secondOwner, secondOwnerInteraction);
         selectorDirected = JavaTckSupport.interactionSet(selector, selectorInteraction);
         sender.publishObjectClassDirectedInteractions(senderClass, senderDirected);
         interactionPublished = true;
         owner.subscribeObjectClassDirectedInteractions(ownerClass, ownerDirected);
         ownerDirectedSubscribed = true;
         secondOwner.subscribeObjectClassDirectedInteractions(secondOwnerClass,
            secondOwnerDirected);
         secondOwnerDirectedSubscribed = true;
         selector.subscribeObjectClassDirectedInteractionsUniversally(selectorClass,
            selectorDirected);
         selectorDirectedSubscribed = true;

         ObjectInstanceHandle ownerTarget = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle secondOwnerTarget = secondOwner.registerObjectInstance(
            secondOwnerClass);
         awaitDiscovery(sender, selector, senderRecorder, selectorRecorder,
            ownerTarget, secondOwnerTarget);

         byte[] value = new byte[] {0x53, 0x55, 0x42, 0x2d, 0x4b};
         byte[] ownerTag = new byte[] {0x4f, 0x57, 0x4e};
         byte[] secondOwnerTag = new byte[] {0x4f, 0x57, 0x52};
         send(sender, senderInteraction, senderParameter, ownerTarget, value, ownerTag);
         awaitCounts(owner, selector, ownerRecorder, selectorRecorder, 1);
         JavaTckSupport.drain(secondOwner);
         JavaTckSupport.check(secondOwnerRecorder.directed.isEmpty(),
            "by-ownership subscriber received a directed send for another owner's object");
         assertRecord(ownerRecorder.directed.get(0), ownerInteraction, ownerTarget,
            ownerParameter, value, ownerTag, producer, owner, "owner-only delivery");
         assertRecord(selectorRecorder.directed.get(0), selectorInteraction, ownerTarget,
            selectorParameter, value, ownerTag, producer, selector, "universal delivery");

         // Adding an empty by-ownership set must not erase the existing universal declaration.
         InteractionClassHandleSet empty =
            selector.getInteractionClassHandleSetFactory().create();
         selector.subscribeObjectClassDirectedInteractions(selectorClass, empty);
         send(sender, senderInteraction, senderParameter, ownerTarget, value, ownerTag);
         awaitCounts(owner, selector, ownerRecorder, selectorRecorder, 2);
         JavaTckSupport.check(secondOwnerRecorder.directed.isEmpty(),
            "empty directed subscription set routed data to a non-owner");

         send(sender, senderInteraction, senderParameter, secondOwnerTarget, value, secondOwnerTag);
         awaitCounts(secondOwner, selector, secondOwnerRecorder, selectorRecorder, 1);
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.directed.size() == 2,
            "by-ownership subscriber received a second owner's target");
         assertRecord(secondOwnerRecorder.directed.get(0), secondOwnerInteraction,
            secondOwnerTarget, secondOwnerParameter, value, secondOwnerTag, producer,
            secondOwner, "second-owner delivery");
         assertRecord(selectorRecorder.directed.get(2), selectorInteraction,
            secondOwnerTarget, selectorParameter, value, secondOwnerTag, producer,
            selector, "universal second-owner delivery");

         // Change the first subscriber to by-ownership and the second to universal.
         selector.unsubscribeObjectClassDirectedInteractions(selectorClass);
         selectorDirectedSubscribed = false;
         selector.subscribeObjectClassDirectedInteractions(selectorClass, selectorDirected);
         selectorDirectedSubscribed = true;
         secondOwner.unsubscribeObjectClassDirectedInteractions(secondOwnerClass,
            secondOwnerDirected);
         secondOwnerDirectedSubscribed = false;
         secondOwner.subscribeObjectClassDirectedInteractionsUniversally(secondOwnerClass,
            secondOwnerDirected);
         secondOwnerDirectedSubscribed = true;

         send(sender, senderInteraction, senderParameter, secondOwnerTarget, value,
            new byte[] {0x55, 0x31});
         awaitCount(secondOwner, secondOwnerRecorder, 2);
         JavaTckSupport.drain(owner);
         JavaTckSupport.drain(selector);
         JavaTckSupport.check(ownerRecorder.directed.size() == 2,
            "by-ownership subscriber received another owner's target after mode change");
         JavaTckSupport.check(selectorRecorder.directed.size() == 3,
            "by-ownership subscriber received another owner's target after mode change");

         send(sender, senderInteraction, senderParameter, ownerTarget, value,
            new byte[] {0x55, 0x32});
         awaitCount(owner, ownerRecorder, 3);
         awaitCount(secondOwner, secondOwnerRecorder, 3);
         JavaTckSupport.check(selectorRecorder.directed.size() == 3,
            "by-ownership subscriber received a non-owned target after mode change");
         assertRecord(ownerRecorder.directed.get(2), ownerInteraction, ownerTarget,
            ownerParameter, value, new byte[] {0x55, 0x32}, producer, owner,
            "switched universal owner delivery");
         assertRecord(secondOwnerRecorder.directed.get(2), secondOwnerInteraction,
            ownerTarget, secondOwnerParameter, value, new byte[] {0x55, 0x32}, producer,
            secondOwner, "switched universal second-owner delivery");
      } finally {
         if (selectorDirectedSubscribed && joined[3]) {
            try { selector.unsubscribeObjectClassDirectedInteractions(selectorClass); }
            catch (Exception ignored) { }
         }
         if (ownerDirectedSubscribed && joined[1]) {
            try { owner.unsubscribeObjectClassDirectedInteractions(ownerClass); }
            catch (Exception ignored) { }
         }
         if (secondOwnerDirectedSubscribed && joined[2]) {
            try { secondOwner.unsubscribeObjectClassDirectedInteractions(secondOwnerClass); }
            catch (Exception ignored) { }
         }
         if (interactionPublished && joined[0]) {
            try { sender.unpublishObjectClassDirectedInteractions(senderClass); }
            catch (Exception ignored) { }
         }
         if (senderSubscribed && joined[0]) {
            try { sender.unsubscribeObjectClass(senderClass); }
            catch (Exception ignored) { }
         }
         if (selectorSubscribed && joined[3]) {
            try { selector.unsubscribeObjectClass(selectorClass); }
            catch (Exception ignored) { }
         }
         if (ownerPublished && joined[1]) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (secondOwnerPublished && joined[2]) {
            try { secondOwner.unpublishObjectClassAttributes(secondOwnerClass,
               secondOwnerAttributes); }
            catch (Exception ignored) { }
         }
         resign(selector, joined[3]);
         resign(secondOwner, joined[2]);
         resign(owner, joined[1]);
         resign(sender, joined[0]);
         if (created) {
            try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         disconnect(selector, connected[3]);
         disconnect(secondOwner, connected[2]);
         disconnect(owner, connected[1]);
         disconnect(sender, connected[0]);
      }
   }

   private static void send(RTIambassador sender, InteractionClassHandle interaction,
         ParameterHandle parameter, ObjectInstanceHandle target, byte[] value, byte[] tag)
         throws Exception {
      ParameterHandleValueMap values = sender.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value);
      sender.sendDirectedInteraction(interaction, target, values, tag);
   }

   private static void awaitDiscovery(RTIambassador sender, RTIambassador selector,
         Recorder senderRecorder, Recorder selectorRecorder, ObjectInstanceHandle first,
         ObjectInstanceHandle second) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (!senderRecorder.discoveries.contains(first)
               || !senderRecorder.discoveries.contains(second)
               || !selectorRecorder.discoveries.contains(first)
               || !selectorRecorder.discoveries.contains(second))) {
         JavaTckSupport.drain(sender);
         JavaTckSupport.drain(selector);
         Thread.yield();
      }
      JavaTckSupport.check(senderRecorder.discoveries.contains(first)
            && senderRecorder.discoveries.contains(second)
            && selectorRecorder.discoveries.contains(first)
            && selectorRecorder.discoveries.contains(second),
         "sender or universal subscriber did not discover both directed targets");
   }

   private static void awaitCounts(RTIambassador first, RTIambassador second,
         Recorder firstRecorder, Recorder secondRecorder, int count) throws Exception {
      awaitCount(first, firstRecorder, count);
      awaitCount(second, secondRecorder, count);
   }

   private static void awaitCount(RTIambassador ambassador, Recorder recorder, int count)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && recorder.directed.size() < count) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(recorder.directed.size() >= count,
         "directed subscription-kind callback did not arrive before deadline");
   }

   private static void assertRecord(Directed received, InteractionClassHandle interaction,
         ObjectInstanceHandle target, ParameterHandle parameter, byte[] value, byte[] tag,
         FederateHandle producer, RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(interaction.equals(received.interaction)
            && target.equals(received.target)
            && received.parameters.size() == 1
            && received.parameters.containsKey(parameter)
            && Arrays.equals(value, received.parameters.get(parameter)),
         description + " carried the wrong interaction, target, or parameter");
      JavaTckSupport.check(Arrays.equals(tag, received.tag),
         description + " did not preserve the tag");
      JavaTckSupport.check("HLAreliable".equals(
            inspector.getTransportationTypeName(received.transportation)),
         description + " did not preserve reliable transportation");
      JavaTckSupport.check(producer.equals(received.producer),
         description + " carried the wrong producer");
   }

   private static void resign(RTIambassador ambassador, boolean joined) {
      if (joined) {
         try { ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
         catch (Exception ignored) { }
      }
   }

   private static void disconnect(RTIambassador ambassador, boolean connected) {
      if (connected) {
         try { ambassador.disconnect(); } catch (Exception ignored) { }
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
                  if ("toString".equals(name)) return "Java TCK subscription-kind callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
