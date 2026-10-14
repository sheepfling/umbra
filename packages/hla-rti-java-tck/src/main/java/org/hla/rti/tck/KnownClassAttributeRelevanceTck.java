package org.hla.rti.tck;

import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;

/** Pure Java/API translation of the adapter-configured known-class advisory cases. */
final class KnownClassAttributeRelevanceTck {
   private KnownClassAttributeRelevanceTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseObjectClassName, String derivedObjectClassName,
         String knownAttributeName, String derivedAttributeName,
         boolean expectedKnownClassPolicy, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-known-class-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-known-class-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-known-class-subscriber", federation);
         subscriberJoined = true;

         JavaTckSupport.check(publisher.getAdvisoriesUseKnownClassSwitch()
               == expectedKnownClassPolicy,
            "adapter FOM known-class policy did not match the requested scenario");
         JavaTckSupport.check(subscriber.getAdvisoriesUseKnownClassSwitch()
               == expectedKnownClassPolicy,
            "joined subscriber reported a different known-class policy");
         JavaTckSupport.check(publisher.getAttributeRelevanceAdvisorySwitch()
               && subscriber.getAttributeRelevanceAdvisorySwitch(),
            "adapter FOM must enable attribute-relevance advisories");

         ObjectClassHandle publisherBase = publisher.getObjectClassHandle(baseObjectClassName);
         ObjectClassHandle publisherDerived =
            publisher.getObjectClassHandle(derivedObjectClassName);
         ObjectClassHandle subscriberBase = subscriber.getObjectClassHandle(baseObjectClassName);
         ObjectClassHandle subscriberDerived =
            subscriber.getObjectClassHandle(derivedObjectClassName);
         AttributeHandle publisherKnown =
            publisher.getAttributeHandle(publisherBase, knownAttributeName);
         AttributeHandle publisherDerivedOnly =
            publisher.getAttributeHandle(publisherDerived, derivedAttributeName);
         AttributeHandle subscriberKnown =
            subscriber.getAttributeHandle(subscriberBase, knownAttributeName);
         AttributeHandle subscriberDerivedOnly =
            subscriber.getAttributeHandle(subscriberDerived, derivedAttributeName);
         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherKnown, publisherDerivedOnly);
         AttributeHandleSet subscriberKnownAttributes =
            JavaTckSupport.attributeSet(subscriber, subscriberKnown);
         AttributeHandleSet subscriberDerivedAttributes =
            JavaTckSupport.attributeSet(subscriber, subscriberDerivedOnly);

         publisher.publishObjectClassAttributes(publisherDerived, publisherAttributes);
         subscriber.subscribeObjectClassAttributes(subscriberBase, subscriberKnownAttributes);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherDerived);
         driveUntil(publisher, subscriber,
            () -> !subscriberRecorder.discoveries.isEmpty(), callbackModel, 3000);
         JavaTckSupport.check(subscriberRecorder.discoveries.size() == 1
               && object.equals(subscriberRecorder.discoveries.get(0)),
            "base-class subscriber did not discover the derived object instance");

         driveFor(publisher, subscriber, callbackModel, 150);
         assertAdvisory(publisherRecorder.updatesOn, 0, object, publisherKnown,
            "initial known-attribute subscription");
         JavaTckSupport.check(publisherRecorder.updatesOn.size() == 1,
            "initial base-class declaration did not produce exactly one update-on advisory");

         subscriber.subscribeObjectClassAttributes(subscriberDerived, subscriberDerivedAttributes);
         if (expectedKnownClassPolicy) {
            driveFor(publisher, subscriber, callbackModel, 150);
            JavaTckSupport.check(publisherRecorder.updatesOn.size() == 1,
               "known-class policy exposed an advisory for a derived-only attribute");
            JavaTckSupport.check(publisherRecorder.updatesOff.isEmpty(),
               "adding a derived-only subscription unexpectedly produced update-off");
         } else {
            driveUntil(publisher, subscriber,
               () -> publisherRecorder.updatesOn.size() >= 2, callbackModel, 3000);
            assertAdvisory(publisherRecorder.updatesOn, 1, object, publisherDerivedOnly,
               "derived-only subscription with known-class policy disabled");
            JavaTckSupport.check(publisherRecorder.updatesOn.size() == 2,
               "derived-only subscription produced duplicate update-on advisories");

            subscriber.unsubscribeObjectClassAttributes(subscriberDerived,
               subscriberDerivedAttributes);
            driveUntil(publisher, subscriber,
               () -> !publisherRecorder.updatesOff.isEmpty(), callbackModel, 3000);
            assertAdvisory(publisherRecorder.updatesOff, 0, object, publisherDerivedOnly,
               "removal of derived-only subscription with known-class policy disabled");
            JavaTckSupport.check(publisherRecorder.updatesOff.size() == 1,
               "derived-only unsubscribe produced duplicate update-off advisories");
         }

         subscriber.unsubscribeObjectClassAttributes(subscriberBase, subscriberKnownAttributes);
         driveUntil(publisher, subscriber,
            () -> publisherRecorder.updatesOff.size()
               == (expectedKnownClassPolicy ? 1 : 2), callbackModel, 3000);
         assertAdvisory(publisherRecorder.updatesOff,
            expectedKnownClassPolicy ? 0 : 1, object, publisherKnown,
            "removal of known base-attribute subscription");
         JavaTckSupport.check(publisherRecorder.updatesOff.size()
               == (expectedKnownClassPolicy ? 1 : 2),
            "base-attribute unsubscribe produced an unexpected update-off count");

         subscriber.unsubscribeObjectClass(subscriberBase);
         subscriber.unsubscribeObjectClass(subscriberDerived);
         publisher.unpublishObjectClassAttributes(publisherDerived, publisherAttributes);
         publisher.unpublishObjectClass(publisherDerived);
      } finally {
         if (subscriberJoined) resign(subscriber);
         if (publisherJoined) resign(publisher);
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (subscriberConnected) disconnect(subscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void assertAdvisory(List<Advisory> advisories, int index,
         ObjectInstanceHandle expectedObject, AttributeHandle expectedAttribute,
         String description) {
      JavaTckSupport.check(advisories.size() > index,
         description + " did not produce an update advisory");
      Advisory advisory = advisories.get(index);
      JavaTckSupport.check(expectedObject.equals(advisory.object)
            && advisory.attributes.size() == 1
            && advisory.attributes.contains(expectedAttribute)
            && advisory.updateRate == null,
         description + " advisory did not preserve the expected object and attribute set");
   }

   private static void driveUntil(RTIambassador publisher, RTIambassador subscriber,
         Condition condition, CallbackModel callbackModel,
         long timeoutMillis) throws Exception {
      long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
      while (!condition.test() && System.nanoTime() < deadline) {
         driveOnce(publisher, subscriber, callbackModel);
      }
   }

   private static void driveFor(RTIambassador publisher, RTIambassador subscriber,
         CallbackModel callbackModel, long durationMillis) throws Exception {
      long deadline = System.nanoTime() + durationMillis * 1_000_000L;
      while (System.nanoTime() < deadline) {
         driveOnce(publisher, subscriber, callbackModel);
      }
   }

   private static void driveOnce(RTIambassador publisher, RTIambassador subscriber,
         CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         publisher.evokeCallback(0.0);
         subscriber.evokeCallback(0.0);
      } else {
         Thread.sleep(2);
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

   private interface Condition {
      boolean test();
   }

   private static final class Advisory {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final String updateRate;

      private Advisory(ObjectInstanceHandle object, AttributeHandleSet attributes,
            String updateRate) {
         this.object = object;
         this.attributes = attributes;
         this.updateRate = updateRate;
      }
   }

   private static final class Recorder {
      private final List<Advisory> updatesOn = new CopyOnWriteArrayList<>();
      private final List<Advisory> updatesOff = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("turnUpdatesOnForObjectInstance".equals(name)) {
                  updatesOn.add(new Advisory((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1],
                     arguments.length == 3 ? (String) arguments[2] : null));
               } else if ("turnUpdatesOffForObjectInstance".equals(name)) {
                  updatesOff.add(new Advisory((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               } else if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK known-class callbacks";
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
