package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.TimeUnit;
import java.util.function.BooleanSupplier;

/** Ordinary attribute reflection must stop after its subscription is withdrawn. */
final class AttributeUnsubscribeStopsReflectionTck {
   private AttributeUnsubscribeStopsReflectionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel,
         long callbackTimeoutMillis) throws Exception {
      JavaTckSupport.check(callbackTimeoutMillis > 0,
         "adapter callback timeout must be positive");
      long timeoutNanos = TimeUnit.MILLISECONDS.toNanos(callbackTimeoutMillis);
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-attribute-unsubscribe-reflection-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle publisherClass = null, subscriberClass = null;
      AttributeHandle publisherAttribute = null, subscriberAttribute = null;
      AttributeHandleSet publisherAttributes = null, subscriberAttributes = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-attribute-unsubscribe-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-attribute-unsubscribe-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         JavaTckSupport.check(publisherClass != null && subscriberClass != null
               && publisherAttribute != null && subscriberAttribute != null,
            "adapter FOM did not resolve the selected class and attribute");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         subscriberAttributes = JavaTckSupport.attributeSet(subscriber, subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null, "registration returned a null object handle");
         await(subscriber, callbackModel, timeoutNanos,
            () -> !subscriberCallbacks.discoveries.isEmpty(), "initial object discovery");
         JavaTckSupport.check(subscriberCallbacks.discoveries.size() == 1
               && object.equals(subscriberCallbacks.discoveries.get(0).object)
               && subscriberClass.equals(subscriberCallbacks.discoveries.get(0).objectClass),
            "subscriber did not discover the registered object in the selected class");

         byte[] firstValue = new byte[] {0x11, 0x22};
         byte[] firstTag = new byte[] {0x51};
         send(publisher, object, publisherAttribute, firstValue, firstTag);
         await(subscriber, callbackModel, timeoutNanos,
            () -> !subscriberCallbacks.reflections.isEmpty(), "initial attribute reflection");
         JavaTckSupport.check(subscriberCallbacks.reflections.size() == 1,
            "ordinary first update did not produce exactly one reflection");
         Reflection first = subscriberCallbacks.reflections.get(0);
         JavaTckSupport.check(first.receiveOrder && object.equals(first.object)
               && first.values.size() == 1
               && Arrays.equals(firstValue, first.values.get(subscriberAttribute))
               && Arrays.equals(firstTag, first.tag),
            "initial reflection did not preserve object, attribute, value, tag, or receive order");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = false;
         send(publisher, object, publisherAttribute,
            new byte[] {0x33, 0x44}, new byte[] {0x52});
         observeNoFurtherReflection(subscriber, callbackModel, timeoutNanos,
            subscriberCallbacks.reflections);
      } finally {
         if (subscriberJoined && subscribed) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published) {
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

   private static void send(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      publisher.updateAttributeValues(object, values, tag.clone());
   }

   private static void await(RTIambassador subscriber, CallbackModel model,
         long timeoutNanos, BooleanSupplier condition, String description) throws Exception {
      long start = System.nanoTime();
      while (!condition.getAsBoolean() && System.nanoTime() - start < timeoutNanos) {
         service(subscriber, model);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void observeNoFurtherReflection(RTIambassador subscriber, CallbackModel model,
         long timeoutNanos, List<Reflection> reflections) throws Exception {
      long start = System.nanoTime();
      while (reflections.size() == 1 && System.nanoTime() - start < timeoutNanos) {
         service(subscriber, model);
      }
      JavaTckSupport.check(reflections.size() == 1,
         "an attribute reflection arrived after its subscription was withdrawn");
   }

   private static void service(RTIambassador subscriber, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         subscriber.evokeMultipleCallbacks(0.0, 0.05);
      } else {
         Thread.sleep(1L);
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final boolean receiveOrder;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, boolean receiveOrder) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.receiveOrder = receiveOrder;
      }
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
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]));
               } else if ("reflectAttributeValues".equals(name)) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     arguments.length == 6));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute unsubscription callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
