package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Tests the registration fence across whole-class unpublication and republication. */
final class ObjectPublicationRegistrationFenceTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ObjectPublicationRegistrationFenceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-object-publication-registration-fence-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle publisherClass = null, subscriberClass = null;
      AttributeHandle publisherAttribute = null, subscriberAttribute = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-publication-fence-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-publication-fence-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         JavaTckSupport.check(publisherClass != null && subscriberClass != null
               && publisherAttribute != null && subscriberAttribute != null,
            "adapter FOM did not resolve object publication-fence handles");
         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         AttributeHandleSet subscriberAttributes = JavaTckSupport.attributeSet(
            subscriber, subscriberAttribute);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = true;

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.unpublishObjectClass(publisherClass);
         published = false;
         ObjectClassHandle registeredClass = publisherClass;
         JavaTckSupport.expectFailure(() -> publisher.registerObjectInstance(registeredClass),
            "ObjectClassNotPublished", "registering after whole-class unpublication");

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "registration after republication returned an invalid object handle");
         String objectName = publisher.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "registration after republication returned an invalid object name");
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.discoveries.stream()
               .anyMatch(event -> objectName.equals(event.objectName)),
            "discovery after republication");
         Discovery discovery = subscriberCallbacks.discoveries.stream()
            .filter(event -> objectName.equals(event.objectName)).findFirst()
            .orElseThrow(() -> new AssertionError("republication discovery was not recorded"));
         ObjectInstanceHandle subscriberObject = subscriber.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(subscriberObject.equals(discovery.object)
               && subscriberClass.equals(discovery.objectClass)
               && producer.equals(discovery.producer),
            "republication returned the wrong discovery identity");
         JavaTckSupport.check(subscriberClass.equals(
               subscriber.getKnownObjectClassHandle(subscriberObject)),
            "republication did not restore stable object identity lookups");
      } finally {
         if (subscriberJoined && subscribed) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass,
               JavaTckSupport.attributeSet(subscriber, subscriberAttribute)); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published) {
            try { publisher.unpublishObjectClassAttributes(publisherClass,
               JavaTckSupport.attributeSet(publisher, publisherAttribute)); }
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
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String objectName;
      private final FederateHandle producer;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String objectName, FederateHandle producer) {
         this.object = object;
         this.objectClass = objectClass;
         this.objectName = objectName;
         this.producer = producer;
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
                     && arguments.length == 4) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2],
                     (FederateHandle) arguments[3]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK publication-fence callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
