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
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Portable IEEE 1516.1-2025 asynchronous-delivery and callback-servicing checks. */
final class AsynchronousDeliveryTck {
   private AsynchronousDeliveryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-asynchronous-delivery-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean receiverTimeConstrained = false;
      try {
         publisher.connect(publisherEvents.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverEvents.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-async-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-async-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         AttributeHandleSet published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         AttributeHandleSet subscribed = receiver.getAttributeHandleSetFactory().create();
         subscribed.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         receiver.subscribeObjectClassAttributes(receiverClass, subscribed);

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(receiver, callbackModel, () -> object.equals(receiverEvents.discovered),
            "object discovery");
         JavaTckSupport.check(receiverEvents.discovered.equals(object),
            "receiver did not discover the registered object");

         receiver.enableTimeConstrained();
         receiverTimeConstrained = true;
         await(receiver, callbackModel, () -> receiverEvents.timeConstrainedEnabled,
            "time constrained enablement");
         JavaTckSupport.check(receiverEvents.timeConstrainedEnabled,
            "timeConstrainedEnabled callback was not serviced");

         byte[] firstTag = new byte[] {0x31, 0x32};
         publisher.updateAttributeValues(object,
            oneValue(publisher, publisherAttribute, (byte) 0x41), firstTag);
         observeStableReflections(receiver, callbackModel, receiverEvents, 0);
         JavaTckSupport.check(receiverEvents.reflections == 0,
            "receive-order reflection passed while asynchronous delivery was disabled");

         receiver.enableAsynchronousDelivery();
         await(receiver, callbackModel, () -> receiverEvents.reflections == 1,
            "first receive-order reflection");
         JavaTckSupport.check(receiverEvents.reflections == 1,
            "enabling asynchronous delivery did not release the queued reflection");
         JavaTckSupport.check(object.equals(receiverEvents.reflectedObject)
               && producer.equals(receiverEvents.reflectionProducer)
               && java.util.Arrays.equals(firstTag, receiverEvents.reflectionTag)
               && receiverEvents.reflectionValues.size() == 1
               && java.util.Arrays.equals(new byte[] {(byte) 0x41},
                  receiverEvents.reflectionValues.get(receiverAttribute)),
            "released reflection did not preserve standard object, producer, and tag data");

         receiver.disableAsynchronousDelivery();
         byte[] secondTag = new byte[] {0x51, 0x52};
         publisher.updateAttributeValues(object,
            oneValue(publisher, publisherAttribute, (byte) 0x42), secondTag);
         observeStableReflections(receiver, callbackModel, receiverEvents, 1);
         JavaTckSupport.check(receiverEvents.reflections == 1,
            "disabling asynchronous delivery did not gate the next reflection");
         receiver.enableAsynchronousDelivery();
         await(receiver, callbackModel, () -> receiverEvents.reflections == 2,
            "second receive-order reflection");
         JavaTckSupport.check(receiverEvents.reflections == 2
               && java.util.Arrays.equals(secondTag, receiverEvents.reflectionTag)
               && receiverEvents.reflectionValues.size() == 1
               && java.util.Arrays.equals(new byte[] {(byte) 0x42},
                  receiverEvents.reflectionValues.get(receiverAttribute)),
            "re-enabling asynchronous delivery did not release the next reflection");
      } finally {
         if (receiverTimeConstrained) {
            try {
               receiver.disableTimeConstrained();
               if (callbackModel == CallbackModel.HLA_EVOKED) {
                  receiver.evokeCallback(0.0);
               }
            } catch (Exception ignored) { }
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
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static AttributeHandleValueMap oneValue(RTIambassador ambassador,
         AttributeHandle attribute, byte value) throws Exception {
      AttributeHandleValueMap result = ambassador.getAttributeHandleValueMapFactory().create(1);
      result.put(attribute, new byte[] {value});
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         pump(ambassador, callbackModel);
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static void observeStableReflections(RTIambassador ambassador,
         CallbackModel callbackModel, Recorder events, int expected) throws Exception {
      long deadline = System.nanoTime() + 100_000_000L;
      while (System.nanoTime() < deadline) {
         pump(ambassador, callbackModel);
         JavaTckSupport.check(events.reflections == expected,
            "receive-order reflection passed while asynchronous delivery was disabled");
      }
   }

   private static void pump(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 0.0);
      } else {
         Thread.sleep(1L);
      }
   }

   private static final class Recorder {
      private volatile ObjectInstanceHandle discovered;
      private ObjectInstanceHandle reflectedObject;
      private FederateHandle reflectionProducer;
      private byte[] reflectionTag;
      private java.util.Map<AttributeHandle, byte[]> reflectionValues = java.util.Map.of();
      private volatile int reflections;
      private volatile boolean timeConstrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discovered = (ObjectInstanceHandle) arguments[0];
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && (arguments.length == 6 || arguments.length == 10)) {
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  AttributeHandleValueMap values = (AttributeHandleValueMap) arguments[1];
                  java.util.Map<AttributeHandle, byte[]> copied = new java.util.HashMap<>();
                  for (AttributeHandle attribute : values.keySet()) {
                     copied.put(attribute, values.get(attribute).clone());
                  }
                  reflectionValues = copied;
                  reflectionTag = ((byte[]) arguments[2]).clone();
                  reflectionProducer = (FederateHandle) arguments[4];
                  ++reflections;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK asynchronous-delivery callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
