package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Covers active, passive, default, reset, and invalid update-rate queries. */
final class UpdateRateQueriesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private UpdateRateQueriesTck() { }

   static void run(RtiFactory factory, String rateFom, String timeImplementation,
         String objectClassName, String attributeName, String rateName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-update-rate-queries-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle subscriberClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandle subscriberAttribute = null;
      try {
         publisher.connect(emptyCallbacks(), callbackModel);
         publisherConnected = true;
         subscriber.connect(recorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, rateFom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-rate-query-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-rate-query-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(subscriberClass)
               && publisherAttribute.equals(subscriberAttribute),
            "adapter rate FOM handles did not retain identity across federates");

         double configuredRate = subscriber.getUpdateRateValue(rateName);
         JavaTckSupport.check(configuredRate > 0.0,
            "adapter named update rate must resolve to a positive value");
         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet subscriberAttributes =
            JavaTckSupport.attributeSet(subscriber, subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         awaitDiscovery(subscriber, callbackModel, recorder, object);

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes,
            rateName);
         subscribed = true;
         JavaTckSupport.check(subscriber.getUpdateRateValueForAttribute(object,
               subscriberAttribute) == configuredRate,
            "active named subscription did not report its configured per-attribute rate");
         JavaTckSupport.check(publisher.getUpdateRateValueForAttribute(object,
               publisherAttribute) == 0.0,
            "publication without subscription reported an attribute update rate");

         String missingRate = rateName + "-java-tck-unknown-" + UUID.randomUUID();
         JavaTckSupport.expectFailure(() -> subscriber.getUpdateRateValue(missingRate),
            "InvalidUpdateRateDesignator", "unknown named update-rate query");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = false;
         requireZero(subscriber, object, subscriberAttribute,
            "unsubscribe did not reset the per-attribute update rate");

         subscriber.subscribeObjectClassAttributesPassively(subscriberClass,
            subscriberAttributes, rateName);
         subscribed = true;
         requireZero(subscriber, object, subscriberAttribute,
            "passive named subscription reported an active update rate");
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = false;

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes, "");
         subscribed = true;
         requireZero(subscriber, object, subscriberAttribute,
            "default-rate subscription reported a named per-attribute rate");

         ObjectInstanceHandle unknownObject =
            JavaTckSupport.invalidHandle(ObjectInstanceHandle.class);
         AttributeHandle knownAttribute = subscriberAttribute;
         JavaTckSupport.expectFailure(() -> subscriber.getUpdateRateValueForAttribute(
               unknownObject, knownAttribute), "ObjectInstanceNotKnown",
            "per-attribute update-rate query for unknown object");
         AttributeHandle invalidAttribute =
            JavaTckSupport.invalidHandle(AttributeHandle.class);
         JavaTckSupport.expectFailure(() -> subscriber.getUpdateRateValueForAttribute(
               object, invalidAttribute), "AttributeNotDefined",
            "per-attribute update-rate query for undefined attribute");
      } finally {
         if (subscriberJoined && subscribed && subscriberClass != null
               && subscriberAttribute != null) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass,
               JavaTckSupport.attributeSet(subscriber, subscriberAttribute)); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherClass != null && publisherAttribute != null) {
            try { publisher.unpublishObjectClassAttributes(publisherClass,
               JavaTckSupport.attributeSet(publisher, publisherAttribute)); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) resign(subscriber, ResignAction.NO_ACTION);
         if (publisherJoined) resign(publisher, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) disconnect(subscriber);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void requireZero(RTIambassador ambassador, ObjectInstanceHandle object,
         AttributeHandle attribute, String message) throws Exception {
      JavaTckSupport.check(ambassador.getUpdateRateValueForAttribute(object, attribute) == 0.0,
         message);
   }

   private static void awaitDiscovery(RTIambassador subscriber, CallbackModel callbackModel,
         Recorder recorder, ObjectInstanceHandle object) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!recorder.discoveries.contains(object) && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(subscriber);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(recorder.discoveries.contains(object),
         "subscriber did not discover the registered object");
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK update-rate callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName())
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
