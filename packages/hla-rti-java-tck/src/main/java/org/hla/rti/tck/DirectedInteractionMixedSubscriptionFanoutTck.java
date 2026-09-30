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

/** P1 directed-interaction fan-out across by-ownership and universal subscriptions. */
final class DirectedInteractionMixedSubscriptionFanoutTck {
   private DirectedInteractionMixedSubscriptionFanoutTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String interactionName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador universalFirst = factory.getRtiAmbassador();
      RTIambassador universalSecond = factory.getRtiAmbassador();
      RTIambassador silent = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder ownerRecorder = new Recorder();
      Recorder universalFirstRecorder = new Recorder();
      Recorder universalSecondRecorder = new Recorder();
      Recorder silentRecorder = new Recorder();
      String federation = "java-tck-directed-mixed-subscription-fanout-" + UUID.randomUUID();
      boolean[] connected = new boolean[5];
      boolean created = false;
      boolean[] joined = new boolean[5];
      boolean publisherObjectPublished = false, ownerObjectPublished = false;
      boolean publisherObjectSubscribed = false, firstObjectSubscribed = false;
      boolean secondObjectSubscribed = false, silentObjectSubscribed = false;
      boolean directedPublished = false, ownerDirectedSubscribed = false;
      boolean firstDirectedSubscribed = false, secondDirectedSubscribed = false;
      ObjectClassHandle publisherClass = null, ownerClass = null;
      ObjectClassHandle firstClass = null, secondClass = null, silentClass = null;
      AttributeHandleSet publisherAttributes = null, ownerAttributes = null;
      AttributeHandleSet firstAttributes = null, secondAttributes = null, silentAttributes = null;
      InteractionClassHandle publisherInteraction = null, ownerInteraction = null;
      InteractionClassHandle firstInteraction = null, secondInteraction = null;
      InteractionClassHandleSet publisherDirected = null, ownerDirected = null;
      InteractionClassHandleSet firstDirected = null, secondDirected = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         connected[0] = true;
         owner.connect(ownerRecorder.proxy(), callbackModel);
         connected[1] = true;
         universalFirst.connect(universalFirstRecorder.proxy(), callbackModel);
         connected[2] = true;
         universalSecond.connect(universalSecondRecorder.proxy(), callbackModel);
         connected[3] = true;
         silent.connect(silentRecorder.proxy(), callbackModel);
         connected[4] = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-mixed-publisher", federation);
         joined[0] = true;
         owner.joinFederationExecution("java-tck-directed-mixed-owner", federation);
         joined[1] = true;
         universalFirst.joinFederationExecution("java-tck-directed-mixed-universal-first",
            federation);
         joined[2] = true;
         universalSecond.joinFederationExecution("java-tck-directed-mixed-universal-second",
            federation);
         joined[3] = true;
         silent.joinFederationExecution("java-tck-directed-mixed-silent", federation);
         joined[4] = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         ownerClass = owner.getObjectClassHandle(objectClassName);
         firstClass = universalFirst.getObjectClassHandle(objectClassName);
         secondClass = universalSecond.getObjectClassHandle(objectClassName);
         silentClass = silent.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle firstAttribute = universalFirst.getAttributeHandle(firstClass,
            attributeName);
         AttributeHandle secondAttribute = universalSecond.getAttributeHandle(secondClass,
            attributeName);
         AttributeHandle silentAttribute = silent.getAttributeHandle(silentClass, attributeName);
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         firstAttributes = JavaTckSupport.attributeSet(universalFirst, firstAttribute);
         secondAttributes = JavaTckSupport.attributeSet(universalSecond, secondAttribute);
         silentAttributes = JavaTckSupport.attributeSet(silent, silentAttribute);

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         firstInteraction = universalFirst.getInteractionClassHandle(interactionName);
         secondInteraction = universalSecond.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle firstParameter = universalFirst.getParameterHandle(firstInteraction,
            parameterName);
         ParameterHandle secondParameter = universalSecond.getParameterHandle(secondInteraction,
            parameterName);

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisherObjectPublished = true;
         publisher.subscribeObjectClassAttributes(publisherClass, publisherAttributes);
         publisherObjectSubscribed = true;
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerObjectPublished = true;
         universalFirst.subscribeObjectClassAttributes(firstClass, firstAttributes);
         firstObjectSubscribed = true;
         universalSecond.subscribeObjectClassAttributes(secondClass, secondAttributes);
         secondObjectSubscribed = true;
         silent.subscribeObjectClassAttributes(silentClass, silentAttributes);
         silentObjectSubscribed = true;

         publisherDirected = JavaTckSupport.interactionSet(publisher, publisherInteraction);
         ownerDirected = JavaTckSupport.interactionSet(owner, ownerInteraction);
         firstDirected = JavaTckSupport.interactionSet(universalFirst, firstInteraction);
         secondDirected = JavaTckSupport.interactionSet(universalSecond, secondInteraction);
         publisher.publishObjectClassDirectedInteractions(publisherClass, publisherDirected);
         directedPublished = true;
         owner.subscribeObjectClassDirectedInteractions(ownerClass, ownerDirected);
         ownerDirectedSubscribed = true;
         universalFirst.subscribeObjectClassDirectedInteractionsUniversally(firstClass,
            firstDirected);
         firstDirectedSubscribed = true;
         universalSecond.subscribeObjectClassDirectedInteractionsUniversally(secondClass,
            secondDirected);
         secondDirectedSubscribed = true;

         ObjectInstanceHandle ownerTarget = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle publisherTarget = publisher.registerObjectInstance(publisherClass);
         awaitDiscovery(publisher, universalFirst, universalSecond, silent,
            publisherRecorder, universalFirstRecorder, universalSecondRecorder, silentRecorder,
            ownerTarget, publisherTarget);
         publisherRecorder.directed.clear();
         ownerRecorder.directed.clear();
         universalFirstRecorder.directed.clear();
         universalSecondRecorder.directed.clear();
         silentRecorder.directed.clear();

         byte[] ownerFirstValue = new byte[] {0x4f, 0x31};
         byte[] ownerFirstTag = new byte[] {0x61};
         byte[] ownerSecondValue = new byte[] {0x4f, 0x32};
         byte[] ownerSecondTag = new byte[] {0x62};
         send(publisher, publisherInteraction, publisherParameter, ownerTarget,
            ownerFirstValue, ownerFirstTag);
         send(publisher, publisherInteraction, publisherParameter, ownerTarget,
            ownerSecondValue, ownerSecondTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(ownerRecorder.directed.isEmpty()
                  && universalFirstRecorder.directed.isEmpty()
                  && universalSecondRecorder.directed.isEmpty(),
               "evoked owner-target directed interactions arrived before callback servicing");
         }
         awaitCounts(owner, universalFirst, universalSecond, ownerRecorder,
            universalFirstRecorder, universalSecondRecorder, 2);
         assertTwo(ownerRecorder.directed, ownerInteraction, ownerTarget, ownerParameter,
            ownerFirstValue, ownerFirstTag, ownerSecondValue, ownerSecondTag, producer,
            owner, "by-ownership subscriber");
         assertTwo(universalFirstRecorder.directed, firstInteraction, ownerTarget, firstParameter,
            ownerFirstValue, ownerFirstTag, ownerSecondValue, ownerSecondTag, producer,
            universalFirst, "first universal subscriber");
         assertTwo(universalSecondRecorder.directed, secondInteraction, ownerTarget,
            secondParameter, ownerFirstValue, ownerFirstTag, ownerSecondValue, ownerSecondTag,
            producer, universalSecond, "second universal subscriber");
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(silent);
         JavaTckSupport.check(publisherRecorder.directed.isEmpty()
               && silentRecorder.directed.isEmpty(),
            "owner-target directed interactions reached the publisher or unsubscribed member");

         byte[] publisherValue = new byte[] {0x50, 0x31};
         byte[] publisherTag = new byte[] {0x71};
         send(publisher, publisherInteraction, publisherParameter, publisherTarget,
            publisherValue, publisherTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(universalFirstRecorder.directed.size() == 2
                  && universalSecondRecorder.directed.size() == 2,
               "evoked publisher-target interaction arrived before callback servicing");
         }
         awaitCounts(universalFirst, universalSecond, universalFirstRecorder,
            universalSecondRecorder, 3);
         JavaTckSupport.drain(owner);
         JavaTckSupport.drain(silent);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(ownerRecorder.directed.size() == 2
               && silentRecorder.directed.isEmpty(),
            "publisher-target interaction escaped universal subscription selection");
         assertRecord(universalFirstRecorder.directed.get(2), firstInteraction, publisherTarget,
            firstParameter, publisherValue, publisherTag, producer, universalFirst,
            "first universal second-target callback");
         assertRecord(universalSecondRecorder.directed.get(2), secondInteraction, publisherTarget,
            secondParameter, publisherValue, publisherTag, producer, universalSecond,
            "second universal second-target callback");
         JavaTckSupport.check(publisherRecorder.directed.isEmpty(),
            "directed interaction was delivered back to its publisher");

         owner.unsubscribeObjectClassDirectedInteractions(ownerClass, ownerDirected);
         ownerDirectedSubscribed = false;
         universalFirst.unsubscribeObjectClassDirectedInteractions(firstClass, firstDirected);
         firstDirectedSubscribed = false;
         universalSecond.unsubscribeObjectClassDirectedInteractions(secondClass, secondDirected);
         secondDirectedSubscribed = false;
         publisher.unpublishObjectClassDirectedInteractions(publisherClass, publisherDirected);
         directedPublished = false;
         publisher.unsubscribeObjectClassAttributes(publisherClass, publisherAttributes);
         publisherObjectSubscribed = false;
         universalFirst.unsubscribeObjectClassAttributes(firstClass, firstAttributes);
         firstObjectSubscribed = false;
         universalSecond.unsubscribeObjectClassAttributes(secondClass, secondAttributes);
         secondObjectSubscribed = false;
         silent.unsubscribeObjectClassAttributes(silentClass, silentAttributes);
         silentObjectSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         publisherObjectPublished = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerObjectPublished = false;
      } finally {
         if (secondDirectedSubscribed && joined[3]) {
            try { universalSecond.unsubscribeObjectClassDirectedInteractions(secondClass,
               secondDirected); } catch (Exception ignored) { }
         }
         if (firstDirectedSubscribed && joined[2]) {
            try { universalFirst.unsubscribeObjectClassDirectedInteractions(firstClass,
               firstDirected); } catch (Exception ignored) { }
         }
         if (ownerDirectedSubscribed && joined[1]) {
            try { owner.unsubscribeObjectClassDirectedInteractions(ownerClass, ownerDirected); }
            catch (Exception ignored) { }
         }
         if (directedPublished && joined[0]) {
            try { publisher.unpublishObjectClassDirectedInteractions(publisherClass,
               publisherDirected); } catch (Exception ignored) { }
         }
         if (silentObjectSubscribed && joined[4]) {
            try { silent.unsubscribeObjectClassAttributes(silentClass, silentAttributes); }
            catch (Exception ignored) { }
         }
         if (secondObjectSubscribed && joined[3]) {
            try { universalSecond.unsubscribeObjectClassAttributes(secondClass, secondAttributes); }
            catch (Exception ignored) { }
         }
         if (firstObjectSubscribed && joined[2]) {
            try { universalFirst.unsubscribeObjectClassAttributes(firstClass, firstAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherObjectSubscribed && joined[0]) {
            try { publisher.unsubscribeObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerObjectPublished && joined[1]) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherObjectPublished && joined[0]) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         resign(silent, joined[4]);
         resign(universalSecond, joined[3]);
         resign(universalFirst, joined[2]);
         resign(owner, joined[1]);
         resign(publisher, joined[0]);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         disconnect(silent, connected[4]);
         disconnect(universalSecond, connected[3]);
         disconnect(universalFirst, connected[2]);
         disconnect(owner, connected[1]);
         disconnect(publisher, connected[0]);
      }
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandle parameter, ObjectInstanceHandle target, byte[] value, byte[] tag)
         throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value);
      publisher.sendDirectedInteraction(interaction, target, values, tag);
   }

   private static void awaitDiscovery(RTIambassador publisher, RTIambassador first,
         RTIambassador second, RTIambassador silent, Recorder publisherRecorder,
         Recorder firstRecorder, Recorder secondRecorder, Recorder silentRecorder,
         ObjectInstanceHandle ownerTarget, ObjectInstanceHandle publisherTarget) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (!publisherRecorder.discoveries.contains(ownerTarget)
               || !firstRecorder.discoveries.contains(ownerTarget)
               || !secondRecorder.discoveries.contains(ownerTarget)
               || !silentRecorder.discoveries.contains(ownerTarget)
               || !firstRecorder.discoveries.contains(publisherTarget)
               || !secondRecorder.discoveries.contains(publisherTarget)
               || !silentRecorder.discoveries.contains(publisherTarget))) {
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         JavaTckSupport.drain(silent);
         Thread.yield();
      }
      JavaTckSupport.check(publisherRecorder.discoveries.contains(ownerTarget)
            && firstRecorder.discoveries.contains(ownerTarget)
            && secondRecorder.discoveries.contains(ownerTarget)
            && silentRecorder.discoveries.contains(ownerTarget)
            && firstRecorder.discoveries.contains(publisherTarget)
            && secondRecorder.discoveries.contains(publisherTarget)
            && silentRecorder.discoveries.contains(publisherTarget),
         "expected members did not discover both directed targets before deadline");
   }

   private static void awaitCounts(RTIambassador owner, RTIambassador first,
         RTIambassador second, Recorder ownerRecorder, Recorder firstRecorder,
         Recorder secondRecorder, int count) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (ownerRecorder.directed.size() < count || firstRecorder.directed.size() < count
               || secondRecorder.directed.size() < count)) {
         JavaTckSupport.drain(owner);
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         Thread.yield();
      }
      JavaTckSupport.check(ownerRecorder.directed.size() >= count
            && firstRecorder.directed.size() >= count
            && secondRecorder.directed.size() >= count,
         "owner and universal directed delivery streams did not arrive before deadline");
   }

   private static void awaitCounts(RTIambassador first, RTIambassador second,
         Recorder firstRecorder, Recorder secondRecorder, int count) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (firstRecorder.directed.size() < count || secondRecorder.directed.size() < count)) {
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         Thread.yield();
      }
      JavaTckSupport.check(firstRecorder.directed.size() >= count
            && secondRecorder.directed.size() >= count,
         "universal directed delivery streams did not arrive before deadline");
   }

   private static void assertTwo(List<Directed> received, InteractionClassHandle interaction,
         ObjectInstanceHandle target, ParameterHandle parameter, byte[] firstValue,
         byte[] firstTag, byte[] secondValue, byte[] secondTag, FederateHandle producer,
         RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(received.size() == 2,
         description + " received an unexpected owner-target callback count");
      assertRecord(received.get(0), interaction, target, parameter, firstValue, firstTag,
         producer, inspector, description + " first callback");
      assertRecord(received.get(1), interaction, target, parameter, secondValue, secondTag,
         producer, inspector, description + " second callback");
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
      String transportationName = received.transportation == null ? null
         : inspector.getTransportationTypeName(received.transportation);
      JavaTckSupport.check(transportationName != null && !transportationName.isEmpty(),
         description + " carried an unknown transportation type");
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
                  if ("toString".equals(name)) return "Java TCK mixed directed callbacks";
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
