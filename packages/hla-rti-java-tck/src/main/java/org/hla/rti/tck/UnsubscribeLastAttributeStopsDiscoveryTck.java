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

/** Portable discovery behavior after removing a class's final attribute subscription. */
final class UnsubscribeLastAttributeStopsDiscoveryTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long NEGATIVE_OBSERVATION_NANOS = 5_000_000_000L;

   private UnsubscribeLastAttributeStopsDiscoveryTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-unsubscribe-last-attribute-" + UUID.randomUUID();
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
         publisher.createFederationExecution(federation, fom);
         created = true;
         publisher.joinFederationExecution("java-tck-unsubscribe-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-unsubscribe-subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle subscriberAttribute =
            subscriber.getAttributeHandle(subscriberClass, attributeName);
         JavaTckSupport.check(publisherClass != null && publisherClass.equals(subscriberClass)
               && publisherAttribute != null && publisherAttribute.equals(subscriberAttribute),
            "adapter FOM did not provide stable object-class and attribute handles");

         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet subscriberAttributes =
            JavaTckSupport.attributeSet(subscriber, subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);

         ObjectInstanceHandle initialObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(initialObject != null,
            "registration before unsubscription returned a null object handle");
         await(subscriber, callbackModel, objectClassName,
            () -> subscriberRecorder.hasDiscovery(initialObject),
            "initial discovery before class-attribute unsubscription");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         ObjectInstanceHandle laterObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(laterObject != null && !laterObject.equals(initialObject),
            "post-unsubscription registration returned a null or reused handle");

         observeNoDiscovery(subscriber, callbackModel, objectClassName,
            subscriberRecorder, laterObject);

         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
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
            try { subscriber.disconnect(); }
            catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(ambassador, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void observeNoDiscovery(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Recorder recorder, ObjectInstanceHandle object) throws Exception {
      long deadline = System.nanoTime() + NEGATIVE_OBSERVATION_NANOS;
      while (System.nanoTime() < deadline && !recorder.hasDiscovery(object)) {
         service(ambassador, model, objectClassName);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(!recorder.hasDiscovery(object),
         "the RTI delivered Discover Object Instance after no class attributes remained subscribed");
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 1.0);
      } else {
         ambassador.getObjectClassHandle(objectClassName);
      }
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK unsubscribe discovery callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private boolean hasDiscovery(ObjectInstanceHandle object) {
         return discoveries.stream().anyMatch(discovered -> object.equals(discovered));
      }
   }
}
