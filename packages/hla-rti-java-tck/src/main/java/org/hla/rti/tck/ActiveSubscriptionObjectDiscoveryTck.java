package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
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
   private ActiveSubscriptionObjectDiscoveryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
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
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-active-discovery-publisher", federation);
         publisherJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         publisherAttributes = publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;

         ObjectInstanceHandle existingObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(existingObject != null,
            "pre-subscription registration returned a null object handle");
         String existingName = publisher.getObjectInstanceName(existingObject);
         JavaTckSupport.check(existingName != null && !existingName.isEmpty(),
            "pre-subscription registration did not return a valid object identity");

         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         subscriber.joinFederationExecution("java-tck-active-discovery-subscriber", federation);
         subscriberJoined = true;
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(
            subscriberClass, attributeName);
         subscriberAttributes = subscriber.getAttributeHandleSetFactory().create();
         subscriberAttributes.add(subscriberAttribute);
         JavaTckSupport.drain(subscriber);
         JavaTckSupport.check(subscriberRecorder.discoveries.isEmpty(),
            "unsubscribed member received discovery for an existing object");

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = true;
         awaitDiscoveryCount(subscriber, subscriberRecorder, 1);
         JavaTckSupport.drain(subscriber);
         Discovery existingDiscovery = findDiscovery(subscriberRecorder.discoveries, existingObject);
         JavaTckSupport.check(subscriberRecorder.discoveries.size() == 1
               && existingDiscovery != null
               && subscriberClass.equals(existingDiscovery.objectClass)
               && existingName.equals(existingDiscovery.name)
               && existingObject.equals(subscriber.getObjectInstanceHandle(existingName))
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
         JavaTckSupport.drain(subscriber);
         Discovery laterDiscovery = findDiscovery(subscriberRecorder.discoveries, laterObject);
         JavaTckSupport.check(subscriberRecorder.discoveries.size() == 2
               && laterDiscovery != null
               && subscriberClass.equals(laterDiscovery.objectClass)
               && laterName.equals(laterDiscovery.name)
               && laterObject.equals(subscriber.getObjectInstanceHandle(laterName))
               && subscriberClass.equals(subscriber.getKnownObjectClassHandle(laterObject)),
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

   private static Discovery findDiscovery(List<Discovery> discoveries,
         ObjectInstanceHandle object) {
      for (Discovery discovery : discoveries) {
         if (object.equals(discovery.object)) return discovery;
      }
      return null;
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private String name;
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
                     && arguments != null && arguments.length >= 3) {
                  Discovery discovery = new Discovery();
                  discovery.object = (ObjectInstanceHandle) arguments[0];
                  discovery.objectClass = (ObjectClassHandle) arguments[1];
                  discovery.name = (String) arguments[2];
                  discoveries.add(discovery);
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
