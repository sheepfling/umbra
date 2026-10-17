package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Checks initial and later value projection when subscribing to an existing object. */
final class SubscriptionInitialReflectionTck {
   private SubscriptionInitialReflectionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstName, String secondName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-subscription-initial-reflection-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle subscriberClass = null;
      AttributeHandle publisherFirst = null;
      AttributeHandle publisherSecond = null;
      AttributeHandle subscriberFirst = null;
      AttributeHandle subscriberSecond = null;
      AttributeHandleSet publishedAttributes = null;
      AttributeHandleSet subscribedAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-initial-reflection-publisher", federation);
         publisherJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         publisherFirst = publisher.getAttributeHandle(publisherClass, firstName);
         publisherSecond = publisher.getAttributeHandle(publisherClass, secondName);
         publishedAttributes = publisher.getAttributeHandleSetFactory().create();
         publishedAttributes.add(publisherFirst);
         publishedAttributes.add(publisherSecond);
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes);
         published = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         byte[] currentFirst = new byte[] {0x21, 0x22};
         byte[] currentSecond = new byte[] {0x31, 0x32};
         AttributeHandleValueMap currentValues =
            publisher.getAttributeHandleValueMapFactory().create(2);
         currentValues.put(publisherFirst, currentFirst);
         currentValues.put(publisherSecond, currentSecond);
         publisher.updateAttributeValues(object, currentValues, new byte[0]);

         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         subscriber.joinFederationExecution("java-tck-initial-reflection-subscriber", federation);
         subscriberJoined = true;
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         subscriberFirst = subscriber.getAttributeHandle(subscriberClass, firstName);
         subscriberSecond = subscriber.getAttributeHandle(subscriberClass, secondName);
         subscribedAttributes = subscriber.getAttributeHandleSetFactory().create();
         subscribedAttributes.add(subscriberFirst);
         JavaTckSupport.drain(subscriber);
         JavaTckSupport.check(subscriberRecorder.discoveredObject == null
               && subscriberRecorder.reflections.isEmpty(),
            "unsubscribed member received discovery or reflection before subscribing");

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscribedAttributes);
         subscribed = true;
         awaitInitialDelivery(subscriber, subscriberRecorder);
         JavaTckSupport.check(object.equals(subscriberRecorder.discoveredObject)
               && subscriberRecorder.reflections.size() == 1,
            "subscription to an existing object did not produce one discovery and initial reflection");
         assertProjection(subscriberRecorder.reflections.get(0), object, subscriberFirst,
            subscriberSecond, currentFirst, "initial reflection");

         byte[] laterFirst = new byte[] {0x41, 0x42};
         byte[] laterSecond = new byte[] {0x51, 0x52};
         AttributeHandleValueMap laterValues =
            publisher.getAttributeHandleValueMapFactory().create(2);
         laterValues.put(publisherFirst, laterFirst);
         laterValues.put(publisherSecond, laterSecond);
         publisher.updateAttributeValues(object, laterValues, new byte[0]);
         awaitReflectionCount(subscriber, subscriberRecorder, 2);
         JavaTckSupport.check(subscriberRecorder.reflections.size() == 2,
            "later multi-attribute update delivered a duplicate reflection");
         assertProjection(subscriberRecorder.reflections.get(1), object, subscriberFirst,
            subscriberSecond, laterFirst, "later reflection");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscribedAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publishedAttributes);
         published = false;
      } finally {
         if (subscribed && subscriberJoined) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscribedAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publishedAttributes); }
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

   private static void awaitInitialDelivery(RTIambassador subscriber, Recorder recorder)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (recorder.discoveredObject == null || recorder.reflections.isEmpty())) {
         JavaTckSupport.drain(subscriber);
         Thread.yield();
      }
      JavaTckSupport.check(recorder.discoveredObject != null && !recorder.reflections.isEmpty(),
         "subscription discovery and initial reflection did not arrive before the bounded deadline");
   }

   private static void awaitReflectionCount(RTIambassador subscriber, Recorder recorder,
         int expectedCount) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && recorder.reflections.size() < expectedCount) {
         JavaTckSupport.drain(subscriber);
         Thread.yield();
      }
      JavaTckSupport.check(recorder.reflections.size() >= expectedCount,
         "reflection count did not reach " + expectedCount + " before the bounded deadline");
   }

   private static void assertProjection(Reflection event, ObjectInstanceHandle object,
         AttributeHandle subscribed, AttributeHandle excluded, byte[] expectedValue,
         String description) {
      JavaTckSupport.check(object.equals(event.object) && event.values.size() == 1
            && event.values.containsKey(subscribed)
            && !event.values.containsKey(excluded)
            && Arrays.equals(expectedValue, event.values.get(subscribed)),
         description + " did not contain exactly the current value for the subscribed attribute");
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
   }

   private static final class Recorder {
      private ObjectInstanceHandle discoveredObject;
      private final List<Reflection> reflections = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 6) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  reflection.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflections.add(reflection);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK subscription initial reflection callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
