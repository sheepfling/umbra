package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks discovery for matching objects present at subscription and registered afterward. */
final class ActiveSubscriptionObjectDiscoveryTck {
   private static final long DUPLICATE_CALLBACK_WINDOW_NANOS = 150_000_000L;

   private ActiveSubscriptionObjectDiscoveryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String secondAttributeName,
         CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-active-subscription-discovery-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle subscriberClass = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet subscriberAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "java-tck-active-discovery-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-active-discovery-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle publisherSecondAttribute = publisher.getAttributeHandle(
            publisherClass, secondAttributeName);
         publisherAttributes = publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         publisherAttributes.add(publisherSecondAttribute);
         JavaTckSupport.check(publisherAttributes.size() == 2,
            "adapter FOM must provide two distinct attributes for this scenario");

         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(
            subscriberClass, attributeName);
         AttributeHandle subscriberSecondAttribute = subscriber.getAttributeHandle(
            subscriberClass, secondAttributeName);
         subscriberAttributes = subscriber.getAttributeHandleSetFactory().create();
         subscriberAttributes.add(subscriberAttribute);
         subscriberAttributes.add(subscriberSecondAttribute);
         JavaTckSupport.check(subscriberAttributes.size() == 2,
            "adapter FOM must provide two distinct attributes for this scenario");

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;

         ObjectInstanceHandle existingObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(existingObject != null,
            "pre-subscription registration returned a null object handle");
         String existingName = publisher.getObjectInstanceName(existingObject);
         JavaTckSupport.check(existingName != null && !existingName.isEmpty(),
            "pre-subscription registration did not return a valid object identity");

         observeNoDiscovery(subscriber, subscriberRecorder);

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = true;
         awaitDiscoveryCount(subscriber, subscriberRecorder, 1);
         settle(subscriber);
         Discovery existingDiscovery = findDiscovery(subscriberRecorder.discoveries, existingName);
         ObjectInstanceHandle existingLocal = subscriber.getObjectInstanceHandle(existingName);
         JavaTckSupport.check(subscriberRecorder.discoveries.size() == 1
               && existingDiscovery != null
               && publisherFederate.equals(existingDiscovery.producer)
               && existingObject.equals(existingLocal)
               && existingObject.equals(existingDiscovery.object)
               && subscriberClass.equals(existingDiscovery.objectClass)
               && existingName.equals(existingDiscovery.name)
               && existingName.equals(subscriber.getObjectInstanceName(existingObject))
               && subscriberClass.equals(subscriber.getKnownObjectClassHandle(existingObject)),
            "active subscription did not produce exactly one matching discovery for the existing object");

         ObjectInstanceHandle laterObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(laterObject != null,
            "post-subscription registration returned a null object handle");
         String laterName = publisher.getObjectInstanceName(laterObject);
         JavaTckSupport.check(!laterObject.equals(existingObject)
               && laterName != null && !laterName.isEmpty() && !laterName.equals(existingName),
            "post-subscription registration did not return a distinct object identity");
         awaitDiscoveryCount(subscriber, subscriberRecorder, 2);
         settle(subscriber);
         Discovery laterDiscovery = findDiscovery(subscriberRecorder.discoveries, laterName);
         ObjectInstanceHandle laterLocal = subscriber.getObjectInstanceHandle(laterName);
         JavaTckSupport.check(subscriberRecorder.discoveries.size() == 2
               && laterDiscovery != null
               && publisherFederate.equals(laterDiscovery.producer)
               && laterObject.equals(laterLocal)
               && laterObject.equals(laterDiscovery.object)
               && subscriberClass.equals(laterDiscovery.objectClass)
               && !laterLocal.equals(existingLocal)
               && laterName.equals(laterDiscovery.name)
               && laterName.equals(subscriber.getObjectInstanceName(laterObject))
               && subscriberClass.equals(subscriber.getKnownObjectClassHandle(laterObject))
               && existingName.equals(subscriber.getObjectInstanceName(existingObject))
               && subscriberClass.equals(subscriber.getKnownObjectClassHandle(existingObject)),
            "active subscription did not discover the later object exactly once with matching identity");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
      } finally {
         if (subscribed && subscriberJoined) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void awaitDiscoveryCount(RTIambassador subscriber, Recorder recorder,
         int expectedCount) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && recorder.discoveries.size() < expectedCount) {
         JavaTckSupport.drain(subscriber);
         Thread.yield();
      }
      JavaTckSupport.check(recorder.discoveries.size() >= expectedCount,
         "discovery count did not reach " + expectedCount + " before the bounded deadline");
   }

   private static void observeNoDiscovery(RTIambassador subscriber, Recorder recorder)
         throws Exception {
      long deadline = System.nanoTime() + 250_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(subscriber);
         JavaTckSupport.check(recorder.discoveries.isEmpty(),
            "unsubscribed member received discovery for an existing object");
         Thread.yield();
      }
   }

   private static void settle(RTIambassador subscriber) throws Exception {
      long deadline = System.nanoTime() + DUPLICATE_CALLBACK_WINDOW_NANOS;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(subscriber);
         Thread.sleep(1L);
      }
   }

   private static Discovery findDiscovery(List<Discovery> discoveries,
         String objectName) {
      for (Discovery discovery : discoveries) {
         if (objectName.equals(discovery.name)) return discovery;
      }
      return null;
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;
      private final FederateHandle producer;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String name, FederateHandle producer) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length == 4) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2],
                     (FederateHandle) arguments[3]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK active subscription discoveries";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
