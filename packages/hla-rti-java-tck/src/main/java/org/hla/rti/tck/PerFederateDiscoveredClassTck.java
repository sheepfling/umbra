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

final class PerFederateDiscoveredClassTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;
   private static final long DUPLICATE_CALLBACK_WINDOW_NANOS = 150_000_000L;

   private PerFederateDiscoveredClassTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseClassName, String derivedClassName, String baseAttributeName,
         String derivedAttributeName, CallbackModel callbackModel) throws Exception {
      runInternal(factory, fom, timeImplementation, baseClassName, derivedClassName,
         baseAttributeName, derivedAttributeName, callbackModel, false);
   }

   static void runRegisteredClassImmutability(RtiFactory factory, String fom,
         String timeImplementation, String baseClassName, String derivedClassName,
         String baseAttributeName, String derivedAttributeName, CallbackModel callbackModel)
         throws Exception {
      runInternal(factory, fom, timeImplementation, baseClassName, derivedClassName,
         baseAttributeName, derivedAttributeName, callbackModel, true);
   }

   private static void runInternal(RtiFactory factory, String fom, String timeImplementation,
         String baseClassName, String derivedClassName, String baseAttributeName,
         String derivedAttributeName, CallbackModel callbackModel,
         boolean verifyRegisteredClassImmutability) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador baseSubscriber = factory.getRtiAmbassador();
      RTIambassador derivedSubscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder baseCallbacks = new Recorder();
      Recorder derivedCallbacks = new Recorder();
      String federation = "java-tck-per-federate-discovered-class-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean baseConnected = false;
      boolean derivedConnected = false;
      boolean federationCreated = false;
      boolean publisherJoined = false;
      boolean baseJoined = false;
      boolean derivedJoined = false;
      boolean published = false;
      boolean baseSubscribed = false;
      boolean baseSubscribedToDerived = false;
      boolean derivedSubscribed = false;
      boolean derivedSubscribedToBase = false;
      ObjectClassHandle publisherDerivedClass = null;
      ObjectClassHandle baseClass = null;
      ObjectClassHandle derivedClass = null;
      ObjectClassHandle baseSubscriberDerivedClass = null;
      ObjectClassHandle derivedSubscriberBaseClass = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet baseAttributes = null;
      AttributeHandleSet baseSubscriberDerivedAttributes = null;
      AttributeHandleSet derivedAttributes = null;
      AttributeHandleSet derivedSubscriberBaseAttributes = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         baseSubscriber.connect(baseCallbacks.proxy(), callbackModel);
         baseConnected = true;
         derivedSubscriber.connect(derivedCallbacks.proxy(), callbackModel);
         derivedConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         publisher.joinFederationExecution("java-tck-per-class-publisher", federation);
         publisherJoined = true;
         baseSubscriber.joinFederationExecution("java-tck-per-class-base-view", federation);
         baseJoined = true;
         derivedSubscriber.joinFederationExecution(
            "java-tck-per-class-derived-view", federation);
         derivedJoined = true;

         publisherDerivedClass = publisher.getObjectClassHandle(derivedClassName);
         ObjectClassHandle publisherBaseClass = publisher.getObjectClassHandle(baseClassName);
         baseClass = baseSubscriber.getObjectClassHandle(baseClassName);
         baseSubscriberDerivedClass = baseSubscriber.getObjectClassHandle(derivedClassName);
         derivedSubscriberBaseClass = derivedSubscriber.getObjectClassHandle(baseClassName);
         derivedClass = derivedSubscriber.getObjectClassHandle(derivedClassName);
         AttributeHandle publisherBaseAttribute = publisher.getAttributeHandle(
            publisherDerivedClass, baseAttributeName);
         AttributeHandle publisherDerivedAttribute = publisher.getAttributeHandle(
            publisherDerivedClass, derivedAttributeName);
         AttributeHandle baseSubscriberAttribute = baseSubscriber.getAttributeHandle(
            baseClass, baseAttributeName);
         AttributeHandle baseSubscriberDerivedAttribute = baseSubscriber.getAttributeHandle(
            baseSubscriberDerivedClass, derivedAttributeName);
         AttributeHandle derivedSubscriberAttribute = derivedSubscriber.getAttributeHandle(
            derivedClass, derivedAttributeName);
         AttributeHandle derivedSubscriberBaseAttribute = derivedSubscriber.getAttributeHandle(
            derivedSubscriberBaseClass, baseAttributeName);
         JavaTckSupport.check(publisherBaseClass != null && publisherDerivedClass != null
               && !publisherBaseClass.equals(publisherDerivedClass)
               && baseClass != null && baseSubscriberDerivedClass != null
               && !baseClass.equals(baseSubscriberDerivedClass)
               && derivedSubscriberBaseClass != null && derivedClass != null
               && !derivedSubscriberBaseClass.equals(derivedClass)
               && publisherBaseAttribute != null && publisherDerivedAttribute != null
               && baseSubscriberAttribute != null && baseSubscriberDerivedAttribute != null
               && derivedSubscriberAttribute != null && derivedSubscriberBaseAttribute != null,
            "adapter hierarchy FOM did not resolve the configured classes and attributes");

         publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherBaseAttribute, publisherDerivedAttribute);
         baseAttributes = JavaTckSupport.attributeSet(baseSubscriber, baseSubscriberAttribute);
         baseSubscriberDerivedAttributes = JavaTckSupport.attributeSet(
            baseSubscriber, baseSubscriberDerivedAttribute);
         derivedAttributes = JavaTckSupport.attributeSet(
            derivedSubscriber, derivedSubscriberAttribute);
         derivedSubscriberBaseAttributes = JavaTckSupport.attributeSet(
            derivedSubscriber, derivedSubscriberBaseAttribute);
         publisher.publishObjectClassAttributes(publisherDerivedClass, publisherAttributes);
         published = true;
         baseSubscriber.subscribeObjectClassAttributes(baseClass, baseAttributes);
         baseSubscribed = true;
         derivedSubscriber.subscribeObjectClassAttributes(derivedClass, derivedAttributes);
         derivedSubscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherDerivedClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "per-federate class registration returned an invalid object handle");
         String objectName = publisher.getObjectInstanceName(object);
         awaitDiscoveries(baseSubscriber, derivedSubscriber, callbackModel,
            baseCallbacks, derivedCallbacks);
         pumpDuplicateCallbackWindow(baseSubscriber, derivedSubscriber, callbackModel);

         JavaTckSupport.check(baseCallbacks.discoveries.size() == 1
               && derivedCallbacks.discoveries.size() == 1
               && publisherCallbacks.discoveries.isEmpty(),
            "registration did not produce exactly one discovery per subscriber");
         Discovery baseDiscovery = baseCallbacks.discoveries.get(0);
         Discovery derivedDiscovery = derivedCallbacks.discoveries.get(0);
         JavaTckSupport.check(baseClass.equals(baseDiscovery.objectClass)
               && derivedClass.equals(derivedDiscovery.objectClass)
               && !baseDiscovery.objectClass.equals(derivedDiscovery.objectClass),
            "the same object did not have a different discovered class in each federate");
         JavaTckSupport.check(objectName.equals(
               baseSubscriber.getObjectInstanceName(baseDiscovery.object))
               && objectName.equals(derivedSubscriber.getObjectInstanceName(derivedDiscovery.object)),
            "subscriber-local object handles did not resolve to the registrant's object name");
         JavaTckSupport.check(baseClass.equals(
               baseSubscriber.getKnownObjectClassHandle(baseDiscovery.object))
               && derivedClass.equals(derivedSubscriber.getKnownObjectClassHandle(derivedDiscovery.object))
               && publisherDerivedClass.equals(publisher.getKnownObjectClassHandle(object)),
            "Get Known Object Class Handle did not preserve each federate's discovered class");
         if (verifyRegisteredClassImmutability) {
            baseSubscriber.subscribeObjectClassAttributes(
               baseSubscriberDerivedClass, baseSubscriberDerivedAttributes);
            baseSubscribedToDerived = true;
            pumpDuplicateCallbackWindow(baseSubscriber, derivedSubscriber, callbackModel);
            JavaTckSupport.check(baseCallbacks.discoveries.size() == 1
                  && baseClass.equals(baseCallbacks.discoveries.get(0).objectClass)
                  && baseClass.equals(baseSubscriber.getKnownObjectClassHandle(baseDiscovery.object))
                  && publisherDerivedClass.equals(publisher.getKnownObjectClassHandle(object)),
               "adding a derived-class subscription changed the base subscriber's known object class");

            derivedSubscriber.unsubscribeObjectClassAttributes(derivedClass, derivedAttributes);
            derivedSubscribed = false;
            derivedSubscriber.subscribeObjectClassAttributes(
               derivedSubscriberBaseClass, derivedSubscriberBaseAttributes);
            derivedSubscribedToBase = true;
            pumpDuplicateCallbackWindow(baseSubscriber, derivedSubscriber, callbackModel);
            JavaTckSupport.check(derivedCallbacks.discoveries.size() == 1
                  && derivedClass.equals(derivedCallbacks.discoveries.get(0).objectClass)
                  && derivedClass.equals(
                     derivedSubscriber.getKnownObjectClassHandle(derivedDiscovery.object))
                  && publisherDerivedClass.equals(publisher.getKnownObjectClassHandle(object)),
               "changing the derived subscriber to a base subscription changed its known object class");
         }
      } finally {
         if (derivedJoined && derivedSubscribedToBase && derivedSubscriberBaseClass != null
               && derivedSubscriberBaseAttributes != null) {
            try { derivedSubscriber.unsubscribeObjectClassAttributes(
               derivedSubscriberBaseClass, derivedSubscriberBaseAttributes); }
            catch (Exception ignored) { }
         }
         if (derivedJoined && derivedSubscribed && derivedClass != null
               && derivedAttributes != null) {
            try { derivedSubscriber.unsubscribeObjectClassAttributes(derivedClass, derivedAttributes); }
            catch (Exception ignored) { }
         }
         if (baseJoined && baseSubscribedToDerived && baseSubscriberDerivedClass != null
               && baseSubscriberDerivedAttributes != null) {
            try { baseSubscriber.unsubscribeObjectClassAttributes(
               baseSubscriberDerivedClass, baseSubscriberDerivedAttributes); }
            catch (Exception ignored) { }
         }
         if (baseJoined && baseSubscribed && baseClass != null && baseAttributes != null) {
            try { baseSubscriber.unsubscribeObjectClassAttributes(baseClass, baseAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherDerivedClass != null
               && publisherAttributes != null) {
            try { publisher.unpublishObjectClassAttributes(publisherDerivedClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (derivedJoined) resign(derivedSubscriber, ResignAction.NO_ACTION);
         if (baseJoined) resign(baseSubscriber, ResignAction.NO_ACTION);
         if (publisherJoined) resign(publisher, ResignAction.DELETE_OBJECTS);
         if (federationCreated) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (derivedConnected) disconnect(derivedSubscriber);
         if (baseConnected) disconnect(baseSubscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void awaitDiscoveries(RTIambassador baseSubscriber,
         RTIambassador derivedSubscriber, CallbackModel callbackModel,
         Recorder baseCallbacks, Recorder derivedCallbacks) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while ((baseCallbacks.discoveries.isEmpty() || derivedCallbacks.discoveries.isEmpty())
            && System.nanoTime() < deadline) {
         serviceCallbacks(baseSubscriber, derivedSubscriber, callbackModel);
      }
      JavaTckSupport.check(!baseCallbacks.discoveries.isEmpty()
            && !derivedCallbacks.discoveries.isEmpty(),
         "timed out waiting for both subscriber-specific object discoveries");
   }

   private static void pumpDuplicateCallbackWindow(RTIambassador baseSubscriber,
         RTIambassador derivedSubscriber, CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + DUPLICATE_CALLBACK_WINDOW_NANOS;
      while (System.nanoTime() < deadline) {
         serviceCallbacks(baseSubscriber, derivedSubscriber, callbackModel);
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) Thread.sleep(1L);
      }
   }

   private static void serviceCallbacks(RTIambassador baseSubscriber,
         RTIambassador derivedSubscriber, CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         baseSubscriber.evokeCallback(0.0);
         derivedSubscriber.evokeCallback(0.0);
      } else {
         Thread.yield();
      }
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
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length == 4) {
                  discoveries.add(new Discovery(
                     (ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK per-federate discovery recorder";
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
