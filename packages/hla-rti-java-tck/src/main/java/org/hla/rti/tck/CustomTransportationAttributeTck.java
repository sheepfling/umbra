package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies ordinary attribute delivery using an adapter-declared FOM transportation type. */
final class CustomTransportationAttributeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private CustomTransportationAttributeTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String transportationName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-custom-transport-attribute-" + UUID.randomUUID();
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
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-custom-transport-attribute-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution(
            "java-tck-custom-transport-attribute-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         TransportationTypeHandle publisherTransportation =
            publisher.getTransportationTypeHandle(transportationName);
         TransportationTypeHandle subscriberTransportation =
            subscriber.getTransportationTypeHandle(transportationName);
         JavaTckSupport.check(objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(subscriber.getObjectClassName(subscriberClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass, publisherAttribute))
               && attributeName.equals(subscriber.getAttributeName(subscriberClass, subscriberAttribute))
               && transportationName.equals(
                  publisher.getTransportationTypeName(publisherTransportation))
               && transportationName.equals(
                  subscriber.getTransportationTypeName(subscriberTransportation)),
            "adapter FOM object, attribute, or transportation names did not round-trip");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         subscriberAttributes = JavaTckSupport.attributeSet(subscriber, subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "custom-transportation attribute registration returned an invalid object handle");
         String objectName = publisher.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && object.equals(publisher.getObjectInstanceHandle(objectName)),
            "registered object name did not round-trip through the standard API");
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.discoveries.size() >= 1,
            "custom-transportation object discovery");
         JavaTckSupport.check(subscriberCallbacks.discoveries.size() == 1,
            "object registration produced an unexpected discovery callback count");
         Discovery discovery = subscriberCallbacks.discoveries.get(0);
         JavaTckSupport.check(object.equals(discovery.object)
               && subscriberClass.equals(discovery.objectClass)
               && objectName.equals(discovery.name) && producer.equals(discovery.producer)
               && object.equals(subscriber.getObjectInstanceHandle(discovery.name))
               && subscriberClass.equals(subscriber.getKnownObjectClassHandle(object)),
            "discovery changed standard object identity or producer metadata");

         byte[] value = new byte[] {(byte) 0xa1, (byte) 0xb2, (byte) 0xc3};
         byte[] tag = new byte[] {0x54, 0x41, 0x47};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value.clone());
         publisher.updateAttributeValues(object, values, tag.clone());
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.reflections.size() >= 1,
            "custom-transportation attribute reflection");
         JavaTckSupport.check(subscriberCallbacks.reflections.size() == 1,
            "ordinary attribute update produced an unexpected reflection callback count");
         Reflection reflection = subscriberCallbacks.reflections.get(0);
         JavaTckSupport.check(object.equals(reflection.object)
               && reflection.values.size() == 1
               && Arrays.equals(value, reflection.values.get(subscriberAttribute))
               && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer)
               && reflection.transportation != null
               && transportationName.equals(
                  subscriber.getTransportationTypeName(reflection.transportation))
               && reflection.regionsEmpty,
            "reflection changed the attribute value, tag, producer, or transportation identity");

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
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
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

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;
      private final FederateHandle producer;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String name, FederateHandle producer) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
         this.producer = producer;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            boolean regionsEmpty) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
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
                     && arguments.length == 4) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2],
                     (FederateHandle) arguments[3]));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  hla.rti1516_2025.RegionHandleSet regions =
                     (hla.rti1516_2025.RegionHandleSet) arguments[5];
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     regions == null || regions.isEmpty()));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK custom attribute callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
