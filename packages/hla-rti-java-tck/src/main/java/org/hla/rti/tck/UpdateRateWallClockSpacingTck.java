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

/** Enforces the named maximum update rate's receive-order wall-clock spacing. */
final class UpdateRateWallClockSpacingTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private UpdateRateWallClockSpacingTck() { }

   static void run(RtiFactory factory, String rateFom, String timeImplementation,
         String objectClassName, String attributeName, String updateRateDesignator,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-update-rate-wall-clock-spacing-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandle receiverAttribute = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet receiverAttributes = null;
      try {
         publisher.connect(new Recorder().proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, rateFom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-update-rate-spacing-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-update-rate-spacing-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute),
            "adapter rate FOM handles did not retain identity across federates");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(
            receiverClass, receiverAttributes, updateRateDesignator);
         subscribed = true;

         double maximumRate = receiver.getUpdateRateValue(updateRateDesignator);
         JavaTckSupport.check(Double.isFinite(maximumRate) && maximumRate > 0.0,
            "named update rate must resolve to a positive finite maximum rate");
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null, "registration returned a null object handle");
         await(receiver, callbackModel,
            () -> receiverRecorder.discoveries.contains(object), "object discovery");
         JavaTckSupport.check(receiver.getUpdateRateValueForAttribute(object, receiverAttribute)
               == maximumRate,
            "named subscription did not select the resolved maximum update rate");

         send(publisher, object, publisherAttribute, new byte[] {0x11});
         await(receiver, callbackModel,
            () -> !receiverRecorder.reflections.isEmpty(), "first receive-order reflection");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 1,
            "first update did not produce exactly one reflection");
         Reflection first = receiverRecorder.reflections.get(0);
         assertReflection(first, object, receiverAttribute, new byte[] {0x11},
            "first receive-order reflection");

         double minimumIntervalNanos = 1_000_000_000.0 / maximumRate;
         send(publisher, object, publisherAttribute, new byte[] {0x12});
         while ((double) (System.nanoTime() - first.callbackEntryNanos) < minimumIntervalNanos) {
            service(receiver, callbackModel);
            verifyObservedSpacing(receiverRecorder.reflections, object, receiverAttribute,
               minimumIntervalNanos);
         }
         service(receiver, callbackModel);
         verifyObservedSpacing(receiverRecorder.reflections, object, receiverAttribute,
            minimumIntervalNanos);
      } finally {
         if (receiverJoined && subscribed) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
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

   private static void send(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      publisher.updateAttributeValues(object, values, new byte[0]);
   }

   private static void assertReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, String description) {
      JavaTckSupport.check(reflection.receiveOrder && reflection.object.equals(object)
            && reflection.values.size() == 1 && reflection.values.containsKey(attribute)
            && Arrays.equals(value, reflection.values.get(attribute)),
         description + " returned the wrong object, value, attribute, or order type");
   }

   private static void verifyObservedSpacing(List<Reflection> reflections,
         ObjectInstanceHandle object, AttributeHandle attribute, double minimumIntervalNanos) {
      for (Reflection reflection : reflections) {
         JavaTckSupport.check(reflection.object.equals(object) && reflection.receiveOrder
               && reflection.values.size() == 1 && reflection.values.containsKey(attribute),
            "update-rate spacing received an unexpected object, attribute, or order type");
         byte[] value = reflection.values.get(attribute);
         JavaTckSupport.check(Arrays.equals(value, new byte[] {0x11})
               || Arrays.equals(value, new byte[] {0x12}),
            "update-rate spacing received an unexpected value");
      }
      for (int index = 1; index < reflections.size(); ++index) {
         long elapsedNanos = reflections.get(index).callbackEntryNanos
            - reflections.get(index - 1).callbackEntryNanos;
         JavaTckSupport.check((double) elapsedNanos >= minimumIntervalNanos,
            "a second reflection callback began before 1/x seconds had elapsed");
      }
   }

   private static void await(RTIambassador receiver, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         service(receiver, callbackModel);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador receiver, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         receiver.evokeMultipleCallbacks(0.0, 0.01);
      } else {
         Thread.sleep(1L);
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final boolean receiveOrder;
      private final long callbackEntryNanos;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            boolean receiveOrder, long callbackEntryNanos) {
         this.object = object;
         this.values = values.clone();
         this.receiveOrder = receiveOrder;
         this.callbackEntryNanos = callbackEntryNanos;
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
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], true, System.nanoTime()));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK update-rate spacing callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
