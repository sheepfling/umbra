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
import java.util.function.BooleanSupplier;

/** Verifies per-attribute active/passive subscription mode transitions. */
final class AttributeSubscriptionModeTransitionsTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private AttributeSubscriptionModeTransitionsTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-attribute-subscription-modes-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false;
      boolean federationCreated = false, publisherJoined = false, subscriberJoined = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle publisherClass = null, subscriberClass = null;
      AttributeHandle publisherFirst = null, publisherSecond = null;
      AttributeHandle subscriberFirst = null, subscriberSecond = null;
      AttributeHandleSet publisherAttributes = null, subscriberAttributes = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         federationCreated = true;
         publisher.joinFederationExecution("java-tck-subscription-mode-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-subscription-mode-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherFirst = publisher.getAttributeHandle(publisherClass, firstAttributeName);
         publisherSecond = publisher.getAttributeHandle(publisherClass, secondAttributeName);
         subscriberFirst = subscriber.getAttributeHandle(subscriberClass, firstAttributeName);
         subscriberSecond = subscriber.getAttributeHandle(subscriberClass, secondAttributeName);
         JavaTckSupport.check(publisherClass != null && subscriberClass != null
               && publisherFirst != null && publisherSecond != null
               && subscriberFirst != null && subscriberSecond != null,
            "adapter FOM did not resolve both class attributes");

         publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherFirst, publisherSecond);
         subscriberAttributes = JavaTckSupport.attributeSet(
            subscriber, subscriberFirst, subscriberSecond);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;

         AttributeHandleSet firstOnly = JavaTckSupport.attributeSet(subscriber, subscriberFirst);
         AttributeHandleSet secondOnly = JavaTckSupport.attributeSet(subscriber, subscriberSecond);
         AttributeHandleSet empty = subscriber.getAttributeHandleSetFactory().create();
         subscriber.subscribeObjectClassAttributes(subscriberClass, firstOnly);
         subscriber.subscribeObjectClassAttributesPassively(subscriberClass, secondOnly);
         subscriber.subscribeObjectClassAttributes(subscriberClass, empty);
         subscriber.subscribeObjectClassAttributesPassively(subscriberClass, empty);
         subscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "subscription-mode registration returned an invalid object handle");
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.discoveries.contains(object), "object discovery");
         JavaTckSupport.check(subscriberCallbacks.discoveries.size() == 1,
            "subscription-mode registration produced an unexpected discovery count");

         byte[] firstValue = {0x4D, 0x31};
         byte[] secondValue = {0x4D, 0x32};
         byte[] tag = {0x4D, 0x41};
         updateBoth(publisher, object, publisherFirst, publisherSecond,
            firstValue, secondValue, tag);
         await(subscriber, callbackModel,
            () -> !subscriberCallbacks.reflections.isEmpty(),
            "reflection for the initially active attribute");
         assertOneAttribute(subscriberCallbacks, object, subscriberFirst,
            subscriberSecond, firstValue, tag, "initial active/passive modes");

         subscriberCallbacks.reflections.clear();
         subscriber.subscribeObjectClassAttributesPassively(subscriberClass, firstOnly);
         subscriber.subscribeObjectClassAttributes(subscriberClass, secondOnly);
         firstValue = new byte[] {0x4D, 0x33};
         secondValue = new byte[] {0x4D, 0x34};
         tag = new byte[] {0x4D, 0x42};
         updateBoth(publisher, object, publisherFirst, publisherSecond,
            firstValue, secondValue, tag);
         await(subscriber, callbackModel,
            () -> !subscriberCallbacks.reflections.isEmpty(),
            "reflection for the newly active attribute");
         assertOneAttribute(subscriberCallbacks, object, subscriberSecond,
            subscriberFirst, secondValue, tag, "selected mode changes");

         subscriberCallbacks.reflections.clear();
         subscriber.subscribeObjectClassAttributes(subscriberClass, firstOnly);
         firstValue = new byte[] {0x4D, 0x35};
         secondValue = new byte[] {0x4D, 0x36};
         tag = new byte[] {0x4D, 0x43};
         updateBoth(publisher, object, publisherFirst, publisherSecond,
            firstValue, secondValue, tag);
         await(subscriber, callbackModel,
            () -> !subscriberCallbacks.reflections.isEmpty(),
            "reflection after reactivating one attribute");
         assertBothAttributes(subscriberCallbacks, object, subscriberFirst,
            subscriberSecond, firstValue, secondValue, tag);

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (subscribed && subscriberJoined && subscriberClass != null
               && subscriberAttributes != null) {
            try { subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined && publisherClass != null && publisherAttributes != null) {
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
         if (federationCreated) {
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

   private static void updateBoth(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle first, AttributeHandle second, byte[] firstValue,
         byte[] secondValue, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
      values.put(first, firstValue.clone());
      values.put(second, secondValue.clone());
      publisher.updateAttributeValues(object, values, tag.clone());
   }

   private static void assertOneAttribute(Recorder recorder, ObjectInstanceHandle object,
         AttributeHandle expected, AttributeHandle excluded, byte[] value, byte[] tag,
         String phase) throws Exception {
      JavaTckSupport.check(recorder.reflections.size() == 1,
         phase + " produced an unexpected reflection count");
      Reflection reflection = recorder.reflections.get(0);
      JavaTckSupport.check(object.equals(reflection.object)
            && reflection.values.size() == 1
            && reflection.values.containsKey(expected)
            && !reflection.values.containsKey(excluded)
            && Arrays.equals(value, reflection.values.get(expected))
            && Arrays.equals(tag, reflection.tag),
         phase + " did not reflect only the expected active attribute");
   }

   private static void assertBothAttributes(Recorder recorder, ObjectInstanceHandle object,
         AttributeHandle first, AttributeHandle second, byte[] firstValue,
         byte[] secondValue, byte[] tag) throws Exception {
      JavaTckSupport.check(recorder.reflections.size() == 1,
         "reactivating one attribute produced an unexpected reflection count");
      Reflection reflection = recorder.reflections.get(0);
      JavaTckSupport.check(object.equals(reflection.object)
            && reflection.values.size() == 2
            && Arrays.equals(firstValue, reflection.values.get(first))
            && Arrays.equals(secondValue, reflection.values.get(second))
            && Arrays.equals(tag, reflection.tag),
         "reactivating one attribute did not preserve the other attribute's active mode");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK subscription mode callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
