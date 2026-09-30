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
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Verifies that withdrawal cancels only callbacks that are still pending. */
final class ReceiveOrderAttributeCallbackCancellationTck {
   private ReceiveOrderAttributeCallbackCancellationTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador active = factory.getRtiAmbassador();
      RTIambassador cancelled = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder activeRecorder = new Recorder();
      Recorder cancelledRecorder = new Recorder();
      String federation = "java-tck-receive-order-attribute-cancel-" + UUID.randomUUID();
      boolean publisherConnected = false, activeConnected = false, cancelledConnected = false;
      boolean created = false, publisherJoined = false, activeJoined = false, cancelledJoined = false;
      boolean activeSubscribed = false, cancelledSubscribed = false;
      ObjectClassHandle publisherClass = null, activeClass = null, cancelledClass = null;
      AttributeHandle publisherAttribute = null, activeAttribute = null, cancelledAttribute = null;
      AttributeHandleSet published = null, activeAttributes = null, cancelledAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         active.connect(activeRecorder.proxy(), callbackModel);
         activeConnected = true;
         cancelled.connect(cancelledRecorder.proxy(), callbackModel);
         cancelledConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-callback-cancel-publisher", federation);
         publisherJoined = true;
         active.joinFederationExecution("java-tck-callback-cancel-active", federation);
         activeJoined = true;
         cancelled.joinFederationExecution("java-tck-callback-cancel-pending", federation);
         cancelledJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         activeClass = active.getObjectClassHandle(objectClassName);
         cancelledClass = cancelled.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         activeAttribute = active.getAttributeHandle(activeClass, attributeName);
         cancelledAttribute = cancelled.getAttributeHandle(
            cancelledClass, attributeName);
         published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         activeAttributes = active.getAttributeHandleSetFactory().create();
         activeAttributes.add(activeAttribute);
         cancelledAttributes = cancelled.getAttributeHandleSetFactory().create();
         cancelledAttributes.add(cancelledAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         active.subscribeObjectClassAttributes(activeClass, activeAttributes);
         activeSubscribed = true;
         cancelled.subscribeObjectClassAttributes(cancelledClass, cancelledAttributes);
         cancelledSubscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         drainAll(publisher, active, cancelled);
         JavaTckSupport.check(object.equals(activeRecorder.discoveredObject)
               && object.equals(cancelledRecorder.discoveredObject),
            "both subscribers must discover the registered object before update delivery");

         byte[] value = new byte[] {0x52, 0x4f, 0x50};
         byte[] tag = new byte[] {0x43, 0x41, 0x4e};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value);
         publisher.updateAttributeValues(object, values, tag);

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(activeRecorder.reflections.isEmpty()
                  && cancelledRecorder.reflections.isEmpty(),
               "evoked receive-order reflections must remain pending until callback servicing");
            cancelled.unsubscribeObjectClassAttributes(cancelledClass, cancelledAttributes);
            cancelledSubscribed = false;
            drainAll(publisher, active, cancelled);
            JavaTckSupport.check(activeRecorder.reflections.size() == 1
                  && cancelledRecorder.reflections.isEmpty(),
               "unsubscription did not cancel only the pending evoked reflection");
         } else {
            awaitBoth(active, cancelled, activeRecorder, cancelledRecorder);
            cancelled.unsubscribeObjectClassAttributes(cancelledClass, cancelledAttributes);
            cancelledSubscribed = false;
            JavaTckSupport.check(activeRecorder.reflections.size() == 1
                  && cancelledRecorder.reflections.size() == 1,
               "immediate mode must deliver both reflections before subscription withdrawal");
         }

         assertReflection(activeRecorder.reflections.get(0), object, activeAttribute,
            value, tag, producer, active, "active subscriber");
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            assertReflection(cancelledRecorder.reflections.get(0), object, cancelledAttribute,
               value, tag, producer, cancelled, "immediate subscriber");
         }
         active.unsubscribeObjectClassAttributes(activeClass, activeAttributes);
         activeSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, published);
      } finally {
         if (cancelledSubscribed && cancelledJoined) {
            try {
               cancelled.unsubscribeObjectClassAttributes(cancelledClass, cancelledAttributes);
            } catch (Exception ignored) { }
         }
         if (activeSubscribed && activeJoined) {
            try {
               active.unsubscribeObjectClassAttributes(activeClass, activeAttributes);
            } catch (Exception ignored) { }
         }
         if (cancelledJoined) {
            try { cancelled.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (activeJoined) {
            try { active.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (cancelledConnected) {
            try { cancelled.disconnect(); } catch (Exception ignored) { }
         }
         if (activeConnected) {
            try { active.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void awaitBoth(RTIambassador active, RTIambassador cancelled,
         Recorder activeRecorder, Recorder cancelledRecorder) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline
            && (activeRecorder.reflections.isEmpty()
               || cancelledRecorder.reflections.isEmpty())) {
         JavaTckSupport.drain(active);
         JavaTckSupport.drain(cancelled);
         Thread.yield();
      }
      JavaTckSupport.check(!activeRecorder.reflections.isEmpty()
            && !cancelledRecorder.reflections.isEmpty(),
         "immediate callbacks did not arrive before the bounded delivery deadline");
   }

   private static void drainAll(RTIambassador publisher, RTIambassador active,
         RTIambassador cancelled) throws Exception {
      JavaTckSupport.drain(publisher);
      JavaTckSupport.drain(active);
      JavaTckSupport.drain(cancelled);
   }

   private static void assertReflection(Reflection event, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag, FederateHandle producer,
         RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(object.equals(event.object) && event.values.size() == 1
            && event.values.containsKey(attribute)
            && Arrays.equals(value, event.values.get(attribute))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty()
            && event.regionsEmpty,
         description + " reflection lost object, attribute, value, tag, producer, or transport metadata");
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
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
                  Reflection event = new Reflection();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  reflections.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK receive-order cancellation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
