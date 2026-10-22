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

/** Distinguish by-ownership and universal directed-interaction subscriptions. */
final class DirectedInteractionSubscriptionKindTck {
   private DirectedInteractionSubscriptionKindTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName,
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
      boolean secondOwnerSubscribed = false;
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
         sender.createFederationExecution(federation, fom, timeImplementation);
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
         JavaTckSupport.check(objectClassName.equals(sender.getObjectClassName(senderClass))
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(secondOwner.getObjectClassName(secondOwnerClass))
               && objectClassName.equals(selector.getObjectClassName(selectorClass))
               && attributeName.equals(sender.getAttributeName(senderClass, senderAttribute))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(secondOwner.getAttributeName(
                  secondOwnerClass, secondOwnerAttribute))
               && attributeName.equals(selector.getAttributeName(selectorClass, selectorAttribute))
               && interactionName.equals(sender.getInteractionClassName(senderInteraction))
               && interactionName.equals(owner.getInteractionClassName(ownerInteraction))
               && interactionName.equals(secondOwner.getInteractionClassName(
                  secondOwnerInteraction))
               && interactionName.equals(selector.getInteractionClassName(selectorInteraction))
               && parameterName.equals(sender.getParameterName(senderInteraction, senderParameter))
               && parameterName.equals(owner.getParameterName(ownerInteraction, ownerParameter))
               && parameterName.equals(secondOwner.getParameterName(
                  secondOwnerInteraction, secondOwnerParameter))
               && parameterName.equals(selector.getParameterName(
                  selectorInteraction, selectorParameter)),
            "adapter FOM class and member names did not round-trip locally");

         sender.subscribeObjectClassAttributes(senderClass, senderAttributes);
         senderSubscribed = true;
         selector.subscribeObjectClassAttributes(selectorClass, selectorAttributes);
         selectorSubscribed = true;
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         secondOwner.publishObjectClassAttributes(secondOwnerClass, secondOwnerAttributes);
         secondOwnerPublished = true;
         secondOwner.subscribeObjectClassAttributes(secondOwnerClass, secondOwnerAttributes);
         secondOwnerSubscribed = true;

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
         String ownerTargetName = owner.getObjectInstanceName(ownerTarget);
         JavaTckSupport.check(ownerTargetName != null && !ownerTargetName.isEmpty(),
            "registered owner target has no standard object name");
         awaitDiscovery(sender, selector, senderRecorder, selectorRecorder,
            ownerTargetName, callbackModel);
         awaitDiscovery(secondOwner, secondOwnerRecorder, ownerTargetName, callbackModel);
         ObjectInstanceHandle selectorOwnerTarget = selector.getObjectInstanceHandle(
            ownerTargetName);
         ObjectInstanceHandle senderOwnerTarget = sender.getObjectInstanceHandle(ownerTargetName);
         ObjectInstanceHandle secondOwnerOwnerTarget = secondOwner.getObjectInstanceHandle(
            ownerTargetName);
         String producerName = sender.getFederateName(producer);
         FederateHandle ownerProducer = owner.getFederateHandle(producerName);
         FederateHandle secondOwnerProducer = secondOwner.getFederateHandle(producerName);
         FederateHandle selectorProducer = selector.getFederateHandle(producerName);
         JavaTckSupport.check(senderRecorder.hasDiscovery(ownerTargetName, senderOwnerTarget)
               && selectorRecorder.hasDiscovery(ownerTargetName, selectorOwnerTarget)
               && secondOwnerRecorder.hasDiscovery(ownerTargetName, secondOwnerOwnerTarget),
            "owner target was not discovered with receiver-local handles");

         byte[] value = new byte[] {0x53, 0x55, 0x42, 0x2d, 0x4b};
         byte[] ownerTag = new byte[] {0x4f, 0x57, 0x4e};
         byte[] secondOwnerTag = new byte[] {0x4f, 0x57, 0x52};
         send(sender, senderInteraction, senderParameter, senderOwnerTarget, value, ownerTag);
         awaitCounts(owner, selector, ownerRecorder, selectorRecorder, 1, 1, callbackModel);
         pumpQuiet(secondOwner, callbackModel);
         JavaTckSupport.check(secondOwnerRecorder.directed.isEmpty(),
            "by-ownership subscriber received a directed send for another owner's object");
         assertRecord(ownerRecorder.directed.get(0), ownerInteraction, ownerTarget,
            ownerParameter, value, ownerTag, ownerProducer, owner, "owner-only delivery");
         assertRecord(selectorRecorder.directed.get(0), selectorInteraction, selectorOwnerTarget,
            selectorParameter, value, ownerTag, selectorProducer, selector, "universal delivery");

         // Adding an empty by-ownership set must not erase the existing universal declaration.
         InteractionClassHandleSet empty =
            selector.getInteractionClassHandleSetFactory().create();
         selector.subscribeObjectClassDirectedInteractions(selectorClass, empty);
         send(sender, senderInteraction, senderParameter, senderOwnerTarget, value, ownerTag);
         awaitCounts(owner, selector, ownerRecorder, selectorRecorder, 2, 2, callbackModel);
         assertRecord(selectorRecorder.directed.get(1), selectorInteraction,
            selectorOwnerTarget, selectorParameter, value, ownerTag, selectorProducer,
            selector, "universal delivery after empty-set subscription");
         pumpQuiet(secondOwner, callbackModel);
         JavaTckSupport.check(secondOwnerRecorder.directed.isEmpty(),
            "empty directed subscription set routed data to a non-owner");

         ObjectInstanceHandle secondOwnerTarget = secondOwner.registerObjectInstance(
            secondOwnerClass);
         String secondOwnerTargetName = secondOwner.getObjectInstanceName(secondOwnerTarget);
         JavaTckSupport.check(secondOwnerTargetName != null && !secondOwnerTargetName.isEmpty(),
            "registered second-owner target has no standard object name");
         awaitDiscovery(sender, selector, senderRecorder, selectorRecorder,
            secondOwnerTargetName, callbackModel);
         ObjectInstanceHandle senderSecondOwnerTarget = sender.getObjectInstanceHandle(
            secondOwnerTargetName);
         ObjectInstanceHandle selectorSecondOwnerTarget = selector.getObjectInstanceHandle(
            secondOwnerTargetName);
         JavaTckSupport.check(senderRecorder.hasDiscovery(secondOwnerTargetName,
               senderSecondOwnerTarget)
               && selectorRecorder.hasDiscovery(secondOwnerTargetName,
                  selectorSecondOwnerTarget),
            "second-owner target was not discovered with receiver-local handles");

         send(sender, senderInteraction, senderParameter, senderSecondOwnerTarget,
            value, secondOwnerTag);
         awaitCounts(secondOwner, selector, secondOwnerRecorder, selectorRecorder,
            1, 3, callbackModel);
         pumpQuiet(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.directed.size() == 2,
            "by-ownership subscriber received a second owner's target");
         assertRecord(secondOwnerRecorder.directed.get(0), secondOwnerInteraction,
            secondOwnerTarget, secondOwnerParameter, value, secondOwnerTag, secondOwnerProducer,
            secondOwner, "second-owner delivery");
         assertRecord(selectorRecorder.directed.get(2), selectorInteraction,
            selectorSecondOwnerTarget, selectorParameter, value, secondOwnerTag, selectorProducer,
            selector, "universal second-owner delivery");

         // Resubscribe the existing selector to by-ownership without first unsubscribing.
         selector.subscribeObjectClassDirectedInteractions(selectorClass, selectorDirected);
         send(sender, senderInteraction, senderParameter, senderOwnerTarget, value,
            new byte[] {0x55, 0x31});
         awaitCount(owner, ownerRecorder, 3, callbackModel);
         assertRecord(ownerRecorder.directed.get(2), ownerInteraction, ownerTarget,
            ownerParameter, value, new byte[] {0x55, 0x31}, ownerProducer, owner,
            "owner delivery after selector resubscription");
         pumpQuiet(selector, callbackModel);
         pumpQuiet(secondOwner, callbackModel);
         JavaTckSupport.check(selectorRecorder.directed.size() == 3,
            "resubscribed by-ownership selector received a non-owned target");
         JavaTckSupport.check(secondOwnerRecorder.directed.size() == 1,
            "unrelated by-ownership subscriber received a non-owned target");

         // Resubscribe the second owner to universal delivery without unsubscribing.
         secondOwner.subscribeObjectClassDirectedInteractionsUniversally(secondOwnerClass,
            secondOwnerDirected);

         send(sender, senderInteraction, senderParameter, senderOwnerTarget, value,
            new byte[] {0x55, 0x32});
         awaitCount(owner, ownerRecorder, 4, callbackModel);
         awaitCount(secondOwner, secondOwnerRecorder, 2, callbackModel);
         pumpQuiet(selector, callbackModel);
         JavaTckSupport.check(selectorRecorder.directed.size() == 3,
            "by-ownership subscriber received a non-owned target after mode change");
         assertRecord(ownerRecorder.directed.get(3), ownerInteraction, ownerTarget,
            ownerParameter, value, new byte[] {0x55, 0x32}, ownerProducer, owner,
            "switched universal owner delivery");
         assertRecord(secondOwnerRecorder.directed.get(1), secondOwnerInteraction,
            secondOwnerOwnerTarget, secondOwnerParameter,
            value, new byte[] {0x55, 0x32}, secondOwnerProducer,
            secondOwner, "switched universal second-owner delivery");
         pumpQuiet(sender, callbackModel);
         pumpQuiet(owner, callbackModel);
         pumpQuiet(secondOwner, callbackModel);
         JavaTckSupport.check(senderRecorder.directed.isEmpty()
               && ownerRecorder.directed.size() == 4
               && secondOwnerRecorder.directed.size() == 2
               && selectorRecorder.directed.size() == 3
               && senderRecorder.unexpectedDirectedCount.get() == 0
               && ownerRecorder.unexpectedDirectedCount.get() == 0
               && secondOwnerRecorder.unexpectedDirectedCount.get() == 0
               && selectorRecorder.unexpectedDirectedCount.get() == 0,
            "directed subscription-kind case produced an unexpected recipient or callback");
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
         if (secondOwnerSubscribed && joined[2]) {
            try { secondOwner.unsubscribeObjectClass(secondOwnerClass); }
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
         Recorder senderRecorder, Recorder selectorRecorder, String targetName,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (!senderRecorder.hasDiscovery(targetName)
               || !selectorRecorder.hasDiscovery(targetName))) {
         service(sender, callbackModel);
         service(selector, callbackModel);
      }
      JavaTckSupport.check(senderRecorder.hasDiscovery(targetName)
            && selectorRecorder.hasDiscovery(targetName),
         "sender or universal subscriber did not discover directed target " + targetName);
   }

   private static void awaitDiscovery(RTIambassador ambassador, Recorder recorder,
         String targetName, CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !recorder.hasDiscovery(targetName)) {
         service(ambassador, callbackModel);
      }
      JavaTckSupport.check(recorder.hasDiscovery(targetName),
         "subscriber did not discover directed target " + targetName);
   }

   private static void awaitCounts(RTIambassador first, RTIambassador second,
         Recorder firstRecorder, Recorder secondRecorder, int firstCount,
         int secondCount, CallbackModel callbackModel) throws Exception {
      awaitCount(first, firstRecorder, firstCount, callbackModel);
      awaitCount(second, secondRecorder, secondCount, callbackModel);
   }

   private static void awaitCount(RTIambassador ambassador, Recorder recorder, int count,
         CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && recorder.directed.size() < count
            && recorder.unexpectedDirectedCount.get() == 0) {
         service(ambassador, callbackModel);
      }
      JavaTckSupport.check(recorder.directed.size() >= count
            && recorder.unexpectedDirectedCount.get() == 0,
         "directed subscription-kind callback count " + recorder.directed.size()
            + " did not reach " + count + " before deadline; unexpected overloads="
            + recorder.unexpectedDirectedCount.get());
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
      String transportation = received.transportation == null ? null
         : inspector.getTransportationTypeName(received.transportation);
      JavaTckSupport.check(transportation != null && !transportation.isEmpty(),
         description + " carried an unknown transportation type");
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
      private final List<Directed> directed = new CopyOnWriteArrayList<>();
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final AtomicInteger unexpectedDirectedCount = new AtomicInteger();

      private boolean hasDiscovery(String name) {
         return discoveries.stream().anyMatch(discovery -> name.equals(discovery.name));
      }

      private boolean hasDiscovery(String name, ObjectInstanceHandle handle) {
         return discoveries.stream().anyMatch(discovery -> name.equals(discovery.name)
            && handle.equals(discovery.handle));
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveDirectedInteraction".equals(name)) {
                  if (arguments != null && arguments.length == 6) {
                     directed.add(new Directed((InteractionClassHandle) arguments[0],
                        (ObjectInstanceHandle) arguments[1],
                        ((ParameterHandleValueMap) arguments[2]).clone(),
                        ((byte[]) arguments[3]).clone(),
                        (TransportationTypeHandle) arguments[4], (FederateHandle) arguments[5]));
                  } else unexpectedDirectedCount.incrementAndGet();
               } else if ("discoverObjectInstance".equals(name)) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (String) arguments[2]));
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

   private static final class Discovery {
      private final ObjectInstanceHandle handle;
      private final String name;

      private Discovery(ObjectInstanceHandle handle, String name) {
         this.handle = handle;
         this.name = name;
      }
   }
}
