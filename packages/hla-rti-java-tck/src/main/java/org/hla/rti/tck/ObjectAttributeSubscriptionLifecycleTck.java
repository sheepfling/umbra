package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Tests passive, active, downgraded, and withdrawn object-attribute subscriptions. */
final class ObjectAttributeSubscriptionLifecycleTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ObjectAttributeSubscriptionLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-object-attribute-subscriptions-" + UUID.randomUUID();
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
            "java-tck-object-subscription-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-object-subscription-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         JavaTckSupport.check(publisherClass != null && subscriberClass != null
               && publisherAttribute != null && subscriberAttribute != null,
            "adapter FOM did not resolve object-attribute subscription lifecycle handles");
         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         AttributeHandleSet subscriberAttributes = JavaTckSupport.attributeSet(
            subscriber, subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;

         subscriber.subscribeObjectClassAttributesPassively(subscriberClass, subscriberAttributes);
         subscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "passive-subscription registration returned an invalid object handle");
         pumpQuiet(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.discoveries.isEmpty(),
            "passive object-attribute subscription unexpectedly discovered an object");

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         await(subscriber, callbackModel,
            () -> !subscriberCallbacks.discoveries.isEmpty(),
            "active object-attribute subscription discovery");
         Discovery discovery = subscriberCallbacks.discoveries.get(0);
         JavaTckSupport.check(object.equals(discovery.object)
               && subscriberClass.equals(discovery.objectClass),
            "active object-attribute subscription returned the wrong discovery identity");
         String objectName = publisher.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && object.equals(subscriber.getObjectInstanceHandle(objectName))
               && subscriberClass.equals(subscriber.getKnownObjectClassHandle(object)),
            "active object-attribute subscription did not establish stable identity lookups");

         byte[] activeValue = new byte[] {0x41, 0x43, 0x54};
         byte[] activeTag = new byte[] {0x41, 0x31};
         sendUpdate(publisher, publisherAttribute, object, activeValue, activeTag);
         await(subscriber, callbackModel, () -> !subscriberCallbacks.reflections.isEmpty(),
            "active object-attribute subscription reflection");
         assertReflection(subscriberCallbacks.reflections.get(0), object, subscriberAttribute,
            activeValue, activeTag, producer, "active object-attribute subscription");

         subscriberCallbacks.reflections.clear();
         subscriber.subscribeObjectClassAttributesPassively(subscriberClass, subscriberAttributes);
         byte[] downgradedValue = new byte[] {0x44, 0x4f, 0x57};
         byte[] downgradedTag = new byte[] {0x44, 0x32};
         sendUpdate(publisher, publisherAttribute, object, downgradedValue, downgradedTag);
         pumpQuiet(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.reflections.isEmpty(),
            "passive object-attribute downgrade did not suppress reflection");
         JavaTckSupport.check(subscriberClass.equals(subscriber.getKnownObjectClassHandle(object)),
            "passive downgrade discarded the known object identity");

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         byte[] reactivatedValue = new byte[] {0x52, 0x45, 0x41};
         byte[] reactivatedTag = new byte[] {0x52, 0x33};
         sendUpdate(publisher, publisherAttribute, object, reactivatedValue, reactivatedTag);
         await(subscriber, callbackModel, () -> !subscriberCallbacks.reflections.isEmpty(),
            "reactivated object-attribute subscription reflection");
         assertReflection(subscriberCallbacks.reflections.get(0), object, subscriberAttribute,
            reactivatedValue, reactivatedTag, producer, "reactivated object-attribute subscription");

         subscriberCallbacks.reflections.clear();
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = false;
         byte[] withdrawnValue = new byte[] {0x57, 0x49, 0x54};
         byte[] withdrawnTag = new byte[] {0x57, 0x34};
         sendUpdate(publisher, publisherAttribute, object, withdrawnValue, withdrawnTag);
         pumpQuiet(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.reflections.isEmpty(),
            "unsubscribed object-attribute declaration unexpectedly reflected an update");
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

   private static void sendUpdate(RTIambassador publisher, AttributeHandle attribute,
         ObjectInstanceHandle object, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value);
      publisher.updateAttributeValues(object, values, tag);
   }

   private static void assertReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag, FederateHandle producer,
         String description) {
      JavaTckSupport.check(object.equals(reflection.object),
         description + " returned the wrong object handle");
      JavaTckSupport.check(reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute)),
         description + " returned the wrong attribute set or value");
      JavaTckSupport.check(Arrays.equals(tag, reflection.tag),
         description + " returned the wrong user tag");
      JavaTckSupport.check(producer.equals(reflection.producer),
         description + " returned the wrong producing federate");
      JavaTckSupport.check(reflection.transportation != null && reflection.transportation.encodedLength() > 0,
         description + " returned an invalid transportation handle");
      JavaTckSupport.check(reflection.regionsEmpty,
         description + " unexpectedly returned source regions");
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

   private static void pumpQuiet(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + 150_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.sleep(2L);
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

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Reflection(ObjectInstanceHandle object, Map<AttributeHandle, byte[]> values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            boolean regionsEmpty) {
         this.object = object;
         this.values = values;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regionsEmpty;
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
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]));
               }
               if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length >= 5) {
                  Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
                  AttributeHandleValueMap map = (AttributeHandleValueMap) arguments[1];
                  for (AttributeHandle attribute : map.keySet()) {
                     values.put(attribute, map.get(attribute).clone());
                  }
                  byte[] tag = arguments[2] instanceof byte[] ? ((byte[]) arguments[2]).clone()
                     : new byte[0];
                  boolean regionsEmpty = arguments.length < 6 || arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0], values, tag,
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     regionsEmpty));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-attribute subscription callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
