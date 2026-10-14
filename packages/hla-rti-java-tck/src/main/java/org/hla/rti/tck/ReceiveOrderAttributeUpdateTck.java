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

/** Verifies consecutive ordinary receive-order attribute updates and callback metadata. */
final class ReceiveOrderAttributeUpdateTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ReceiveOrderAttributeUpdateTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-receive-order-attribute-update-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandle publisherAttribute = null, receiverAttribute = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-receive-order-attribute-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-receive-order-attribute-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherAttribute != null && receiverAttribute != null,
            "adapter FOM did not resolve receive-order attribute-update handles");
         JavaTckSupport.check(objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(receiver.getObjectClassName(receiverClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass, publisherAttribute))
               && attributeName.equals(receiver.getAttributeName(receiverClass, receiverAttribute)),
            "receive-order attribute-update lookup did not round-trip adapter names");

         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         AttributeHandleSet receiverAttributes = JavaTckSupport.attributeSet(
            receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "receive-order attribute-update registration returned an invalid handle");
         await(receiver, callbackModel, () -> receiverCallbacks.discoveries.contains(object),
            "receive-order attribute-update discovery");
         receiverCallbacks.reflections.clear();
         publisherCallbacks.reflections.clear();

         byte[] firstValue = new byte[] {0x52, 0x31};
         byte[] firstTag = new byte[] {0x54, 0x31};
         byte[] secondValue = new byte[] {0x52, 0x32};
         byte[] secondTag = new byte[] {0x54, 0x32};
         send(publisher, object, publisherAttribute, firstValue, firstTag);
         send(publisher, object, publisherAttribute, secondValue, secondTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(receiverCallbacks.reflections.isEmpty(),
               "evoked receive-order updates arrived before callback servicing");
         }
         await(receiver, callbackModel, () -> receiverCallbacks.reflections.size() >= 2,
            "receive-order attribute-update reflections");
         JavaTckSupport.check(receiverCallbacks.reflections.size() == 2,
            "receive-order attribute update delivered an unexpected callback count");
         assertDelivery(receiver, receiverCallbacks.reflections.get(0), object,
            receiverAttribute, firstValue, firstTag, producer, "first receive-order update");
         assertDelivery(receiver, receiverCallbacks.reflections.get(1), object,
            receiverAttribute, secondValue, secondTag, producer, "second receive-order update");
         JavaTckSupport.check(publisherCallbacks.reflections.isEmpty(),
            "receive-order attribute update looped a reflection back to its publisher");
      } finally {
         if (receiverJoined && subscribed) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass,
               JavaTckSupport.attributeSet(receiver, receiverAttribute)); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published) {
            try { publisher.unpublishObjectClassAttributes(publisherClass,
               JavaTckSupport.attributeSet(publisher, publisherAttribute)); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
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

   private static void assertDelivery(RTIambassador receiver, Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value, byte[] tag,
         FederateHandle producer, String description) throws Exception {
      JavaTckSupport.check(!reflection.timestampOrdered && object.equals(reflection.object)
            && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag)
            && producer.equals(reflection.producer)
            && reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty(),
         description + " returned incorrect object, attribute, value, tag, producer, or transport");
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

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean timestampOrdered;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            boolean timestampOrdered) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.timestampOrdered = timestampOrdered;
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
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name)) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     arguments.length != 6));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK receive-order attribute callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
