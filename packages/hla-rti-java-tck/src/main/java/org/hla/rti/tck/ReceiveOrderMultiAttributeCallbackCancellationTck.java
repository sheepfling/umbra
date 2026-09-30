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

/** Verifies multi-attribute receive-order projection and pending-callback cancellation. */
final class ReceiveOrderMultiAttributeCallbackCancellationTck {
   private ReceiveOrderMultiAttributeCallbackCancellationTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String firstName, String secondName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador active = factory.getRtiAmbassador();
      RTIambassador cancelled = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder activeRecorder = new Recorder();
      Recorder cancelledRecorder = new Recorder();
      String federation = "java-tck-receive-order-multi-attribute-cancel-" + UUID.randomUUID();
      boolean publisherConnected = false, activeConnected = false, cancelledConnected = false;
      boolean created = false, publisherJoined = false, activeJoined = false, cancelledJoined = false;
      boolean activeSubscribed = false, cancelledSubscribed = false;
      ObjectClassHandle publisherClass = null, activeClass = null, cancelledClass = null;
      AttributeHandle publisherFirst = null, publisherSecond = null;
      AttributeHandle activeFirst = null, activeSecond = null, cancelledSecond = null;
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
            "java-tck-multi-cancel-publisher", federation);
         publisherJoined = true;
         active.joinFederationExecution("java-tck-multi-cancel-active", federation);
         activeJoined = true;
         cancelled.joinFederationExecution("java-tck-multi-cancel-pending", federation);
         cancelledJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         activeClass = active.getObjectClassHandle(objectClassName);
         cancelledClass = cancelled.getObjectClassHandle(objectClassName);
         publisherFirst = publisher.getAttributeHandle(publisherClass, firstName);
         publisherSecond = publisher.getAttributeHandle(publisherClass, secondName);
         activeFirst = active.getAttributeHandle(activeClass, firstName);
         activeSecond = active.getAttributeHandle(activeClass, secondName);
         cancelledSecond = cancelled.getAttributeHandle(cancelledClass, secondName);
         published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherFirst);
         published.add(publisherSecond);
         activeAttributes = active.getAttributeHandleSetFactory().create();
         activeAttributes.add(activeFirst);
         activeAttributes.add(activeSecond);
         cancelledAttributes = cancelled.getAttributeHandleSetFactory().create();
         cancelledAttributes.add(cancelledSecond);
         publisher.publishObjectClassAttributes(publisherClass, published);
         active.subscribeObjectClassAttributes(activeClass, activeAttributes);
         activeSubscribed = true;
         cancelled.subscribeObjectClassAttributes(cancelledClass, cancelledAttributes);
         cancelledSubscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         drainAll(publisher, active, cancelled);
         JavaTckSupport.check(object.equals(activeRecorder.discoveredObject)
               && object.equals(cancelledRecorder.discoveredObject),
            "both subscribers must discover the registered object before updates");

         byte[] firstValue = new byte[] {0x41, 0x31};
         byte[] secondValue = new byte[] {0x42, 0x32};
         byte[] secondUpdateValue = new byte[] {0x41, 0x32};
         byte[] tag = new byte[] {0x4d, 0x41, 0x43};
         AttributeHandleValueMap both = publisher.getAttributeHandleValueMapFactory().create(2);
         both.put(publisherFirst, firstValue);
         both.put(publisherSecond, secondValue);
         publisher.updateAttributeValues(object, both, tag);

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(activeRecorder.reflections.isEmpty()
                  && cancelledRecorder.reflections.isEmpty(),
               "evoked receive-order reflections must remain pending until callback servicing");
            cancelled.unsubscribeObjectClassAttributes(cancelledClass, cancelledAttributes);
            cancelledSubscribed = false;
            drainAll(publisher, active, cancelled);
            JavaTckSupport.check(activeRecorder.reflections.size() == 1
                  && cancelledRecorder.reflections.isEmpty(),
               "unsubscription did not cancel only the pending evoked multi-attribute reflection");
         } else {
            awaitBoth(active, cancelled, activeRecorder, cancelledRecorder);
            cancelled.unsubscribeObjectClassAttributes(cancelledClass, cancelledAttributes);
            cancelledSubscribed = false;
            JavaTckSupport.check(activeRecorder.reflections.size() == 1
                  && cancelledRecorder.reflections.size() == 1,
               "immediate mode must deliver both projected reflections before withdrawal");
         }

         assertProjection(activeRecorder.reflections.get(0), object, activeFirst, firstValue,
            activeSecond, secondValue, tag, producer, active, "active subscriber");
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            assertProjection(cancelledRecorder.reflections.get(0), object, null, null,
               cancelledSecond, secondValue, tag, producer, cancelled, "immediate subscriber");
         }
         JavaTckSupport.check(publisherRecorder.reflections.isEmpty(),
            "publisher must not receive its own attribute updates as reflections");

         AttributeHandleValueMap firstOnly = publisher.getAttributeHandleValueMapFactory().create(1);
         firstOnly.put(publisherFirst, secondUpdateValue);
         publisher.updateAttributeValues(object, firstOnly, tag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(active);
         } else {
            awaitCount(active, activeRecorder, 2);
         }
         JavaTckSupport.check(activeRecorder.reflections.size() == 2
               && cancelledRecorder.reflections.size()
                  == (callbackModel == CallbackModel.HLA_IMMEDIATE ? 1 : 0),
            "updates after withdrawal must reach only attributes still actively subscribed");
         assertProjection(activeRecorder.reflections.get(1), object, activeFirst,
            secondUpdateValue, null, null, tag, producer, active, "active first-only update");
         JavaTckSupport.check(publisherRecorder.reflections.isEmpty(),
            "publisher must not receive its own later attribute update as a reflection");

         active.unsubscribeObjectClassAttributes(activeClass, activeAttributes);
         activeSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, published);
      } finally {
         if (cancelledSubscribed && cancelledJoined) {
            try { cancelled.unsubscribeObjectClassAttributes(cancelledClass, cancelledAttributes); }
            catch (Exception ignored) { }
         }
         if (activeSubscribed && activeJoined) {
            try { active.unsubscribeObjectClassAttributes(activeClass, activeAttributes); }
            catch (Exception ignored) { }
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
            && (activeRecorder.reflections.isEmpty() || cancelledRecorder.reflections.isEmpty())) {
         JavaTckSupport.drain(active);
         JavaTckSupport.drain(cancelled);
         Thread.yield();
      }
      JavaTckSupport.check(!activeRecorder.reflections.isEmpty()
            && !cancelledRecorder.reflections.isEmpty(),
         "immediate callbacks did not arrive before the bounded delivery deadline");
   }

   private static void awaitCount(RTIambassador ambassador, Recorder recorder, int count)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && recorder.reflections.size() < count) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(recorder.reflections.size() >= count,
         "immediate callback count did not reach " + count + " before deadline");
   }

   private static void drainAll(RTIambassador publisher, RTIambassador active,
         RTIambassador cancelled) throws Exception {
      JavaTckSupport.drain(publisher);
      JavaTckSupport.drain(active);
      JavaTckSupport.drain(cancelled);
   }

   private static void assertProjection(Reflection event, ObjectInstanceHandle object,
         AttributeHandle first, byte[] firstValue, AttributeHandle second, byte[] secondValue,
         byte[] tag, FederateHandle producer, RTIambassador inspector, String description)
         throws Exception {
      boolean firstMatches = first == null
         ? !event.values.containsKey(first)
         : event.values.containsKey(first) && Arrays.equals(firstValue, event.values.get(first));
      boolean secondMatches = second == null
         ? !event.values.containsKey(second)
         : event.values.containsKey(second) && Arrays.equals(secondValue, event.values.get(second));
      int expectedSize = (first == null ? 0 : 1) + (second == null ? 0 : 1);
      JavaTckSupport.check(object.equals(event.object) && event.values.size() == expectedSize
            && firstMatches && secondMatches && Arrays.equals(tag, event.tag)
            && producer.equals(event.producer) && event.transportation != null
            && "HLAreliable".equals(inspector.getTransportationTypeName(event.transportation))
            && event.regionsEmpty,
         description + " reflection did not preserve exact attribute projection or metadata");
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
                  if ("toString".equals(name)) return "Java TCK multi-attribute callback cancellation";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
