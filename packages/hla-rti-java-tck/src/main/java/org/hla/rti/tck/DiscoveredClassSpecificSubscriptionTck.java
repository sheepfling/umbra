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

/** API-only check that a later, more-specific subscription doesn't rewrite a discovery. */
final class DiscoveredClassSpecificSubscriptionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;
   private static final long DUPLICATE_CALLBACK_WINDOW_NANOS = 150_000_000L;

   private DiscoveredClassSpecificSubscriptionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseClassName, String derivedClassName, String baseAttributeName,
         String derivedAttributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-discovered-class-specific-subscription-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean federationCreated = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean published = false;
      boolean baseSubscribed = false;
      boolean derivedSubscribed = false;
      ObjectClassHandle publisherDerivedClass = null;
      ObjectClassHandle subscriberBaseClass = null;
      ObjectClassHandle subscriberDerivedClass = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet subscriberBaseAttributes = null;
      AttributeHandleSet subscriberDerivedAttributes = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         publisher.joinFederationExecution("java-tck-specific-subscription-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-specific-subscription-subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherBaseClass = publisher.getObjectClassHandle(baseClassName);
         publisherDerivedClass = publisher.getObjectClassHandle(derivedClassName);
         subscriberBaseClass = subscriber.getObjectClassHandle(baseClassName);
         subscriberDerivedClass = subscriber.getObjectClassHandle(derivedClassName);
         AttributeHandle publisherBaseAttribute = publisher.getAttributeHandle(
            publisherDerivedClass, baseAttributeName);
         AttributeHandle publisherDerivedAttribute = publisher.getAttributeHandle(
            publisherDerivedClass, derivedAttributeName);
         AttributeHandle subscriberBaseAttribute = subscriber.getAttributeHandle(
            subscriberBaseClass, baseAttributeName);
         AttributeHandle subscriberDerivedAttribute = subscriber.getAttributeHandle(
            subscriberDerivedClass, derivedAttributeName);
         JavaTckSupport.check(publisherBaseClass != null && publisherDerivedClass != null
               && !publisherBaseClass.equals(publisherDerivedClass)
               && subscriberBaseClass != null && subscriberDerivedClass != null
               && !subscriberBaseClass.equals(subscriberDerivedClass)
               && publisherBaseAttribute != null && publisherDerivedAttribute != null
               && subscriberBaseAttribute != null && subscriberDerivedAttribute != null,
            "adapter hierarchy FOM did not resolve the configured classes and attributes");

         publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherBaseAttribute, publisherDerivedAttribute);
         subscriberBaseAttributes = JavaTckSupport.attributeSet(subscriber, subscriberBaseAttribute);
         subscriberDerivedAttributes = JavaTckSupport.attributeSet(
            subscriber, subscriberDerivedAttribute);
         publisher.publishObjectClassAttributes(publisherDerivedClass, publisherAttributes);
         published = true;
         subscriber.subscribeObjectClassAttributes(subscriberBaseClass, subscriberBaseAttributes);
         baseSubscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherDerivedClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "specific-subscription registration returned an invalid object handle");
         String objectName = publisher.getObjectInstanceName(object);
         awaitDiscovery(subscriber, callbackModel, subscriberCallbacks);
         pumpDuplicateCallbackWindow(subscriber, callbackModel);
         Discovery discovery = subscriberCallbacks.discoveries.get(0);
         assertBaseClassStable(publisher, subscriber, object, objectName, publisherDerivedClass,
            subscriberBaseClass, discovery, subscriberCallbacks, publisherCallbacks);

         subscriber.subscribeObjectClassAttributes(
            subscriberDerivedClass, subscriberDerivedAttributes);
         derivedSubscribed = true;
         pumpDuplicateCallbackWindow(subscriber, callbackModel);
         assertBaseClassStable(publisher, subscriber, object, objectName, publisherDerivedClass,
            subscriberBaseClass, discovery, subscriberCallbacks, publisherCallbacks);
      } finally {
         if (subscriberJoined && derivedSubscribed && subscriberDerivedClass != null
               && subscriberDerivedAttributes != null) {
            try { subscriber.unsubscribeObjectClassAttributes(
               subscriberDerivedClass, subscriberDerivedAttributes); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined && baseSubscribed && subscriberBaseClass != null
               && subscriberBaseAttributes != null) {
            try { subscriber.unsubscribeObjectClassAttributes(
               subscriberBaseClass, subscriberBaseAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherDerivedClass != null
               && publisherAttributes != null) {
            try { publisher.unpublishObjectClassAttributes(
               publisherDerivedClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) resign(subscriber, ResignAction.NO_ACTION);
         if (publisherJoined) resign(publisher, ResignAction.DELETE_OBJECTS);
         if (federationCreated) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) disconnect(subscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void assertBaseClassStable(RTIambassador publisher, RTIambassador subscriber,
         ObjectInstanceHandle publisherObject, String objectName,
         ObjectClassHandle publisherDerivedClass, ObjectClassHandle subscriberBaseClass,
         Discovery discovery, Recorder callbacks, Recorder publisherCallbacks) throws Exception {
      JavaTckSupport.check(callbacks.discoveries.size() == 1
            && subscriberBaseClass.equals(discovery.objectClass)
            && objectName.equals(subscriber.getObjectInstanceName(discovery.object))
            && subscriberBaseClass.equals(subscriber.getKnownObjectClassHandle(discovery.object))
            && publisherDerivedClass.equals(publisher.getKnownObjectClassHandle(publisherObject))
            && publisherCallbacks.discoveries.isEmpty(),
         "a more-specific subscription changed the existing discovered class or duplicated discovery");
   }

   private static void awaitDiscovery(RTIambassador subscriber, CallbackModel callbackModel,
         Recorder callbacks) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (callbacks.discoveries.isEmpty() && System.nanoTime() < deadline) {
         serviceCallbacks(subscriber, callbackModel);
      }
      JavaTckSupport.check(!callbacks.discoveries.isEmpty(),
         "timed out waiting for the base-class object discovery");
   }

   private static void pumpDuplicateCallbackWindow(RTIambassador subscriber,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + DUPLICATE_CALLBACK_WINDOW_NANOS;
      while (System.nanoTime() < deadline) {
         serviceCallbacks(subscriber, callbackModel);
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) Thread.sleep(1L);
      }
   }

   private static void serviceCallbacks(RTIambassador subscriber,
         CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) subscriber.evokeCallback(0.0);
      else Thread.yield();
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass) {
         this.object = object;
         this.objectClass = objectClass;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2
                     && arguments[0] instanceof ObjectInstanceHandle
                     && arguments[1] instanceof ObjectClassHandle) {
                  discoveries.add(new Discovery(
                     (ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK discovery recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
