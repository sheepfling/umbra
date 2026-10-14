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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks independent named-rate and default-rate ordinary attribute reflections. */
final class MixedUpdateRateSubscriptionsTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private MixedUpdateRateSubscriptionsTck() { }

   static void run(RtiFactory factory, String rateFom, String timeImplementation,
         String objectClassName, String reliableAttributeName, String bestEffortAttributeName,
         String updateRateDesignator, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-mixed-update-rate-subscriptions-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandle publisherReliable = null;
      AttributeHandle publisherBestEffort = null;
      AttributeHandle receiverReliable = null;
      AttributeHandle receiverBestEffort = null;
      try {
         publisher.connect(new EmptyFederateAmbassador().proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, rateFom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-mixed-rate-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-mixed-rate-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         publisherReliable = publisher.getAttributeHandle(publisherClass, reliableAttributeName);
         publisherBestEffort =
            publisher.getAttributeHandle(publisherClass, bestEffortAttributeName);
         receiverReliable = receiver.getAttributeHandle(receiverClass, reliableAttributeName);
         receiverBestEffort = receiver.getAttributeHandle(receiverClass, bestEffortAttributeName);
         JavaTckSupport.check(!publisherReliable.equals(publisherBestEffort)
               && !receiverReliable.equals(receiverBestEffort),
            "adapter FOM mapped reliable and best-effort attributes to one handle");

         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherReliable, publisherBestEffort);
         AttributeHandleSet receiverAllAttributes = JavaTckSupport.attributeSet(
            receiver, receiverReliable, receiverBestEffort);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverReliable), "");
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverBestEffort), updateRateDesignator);
         subscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(receiver, callbackModel, () -> receiverRecorder.discoveries.contains(object),
            "mixed update-rate object discovery");

         double namedRate = receiver.getUpdateRateValue(updateRateDesignator);
         JavaTckSupport.check(namedRate > 0.0
               && receiver.getUpdateRateValueForAttribute(object, receiverBestEffort) == namedRate,
            "named best-effort subscription did not select its positive update rate");
         JavaTckSupport.check(receiver.getUpdateRateValueForAttribute(object, receiverReliable)
               == 0.0,
            "default reliable subscription unexpectedly selected the named update rate");
         final AttributeHandle reliableHandle = receiverReliable;
         final AttributeHandle bestEffortHandle = receiverBestEffort;

         byte[] firstReliable = new byte[] {0x31, 0x32};
         byte[] firstBestEffort = new byte[] {0x41, 0x42};
         byte[] secondReliable = new byte[] {0x51, 0x52};
         byte[] secondBestEffort = new byte[] {0x61, 0x62};
         byte[] firstTag = new byte[] {0x71};
         byte[] secondTag = new byte[] {0x72};

         int firstStart = receiverRecorder.reflections.size();
         send(publisher, object, publisherReliable, firstReliable,
            publisherBestEffort, firstBestEffort, firstTag);
         await(receiver, callbackModel, () -> count(receiverRecorder.reflections,
               reliableHandle) >= 1 && count(receiverRecorder.reflections,
               bestEffortHandle) >= 1, "first mixed-rate reflection passel");
         JavaTckSupport.check(receiverRecorder.reflections.size() > firstStart,
            "first update produced no reflection callback");
         assertPassel(receiver, receiverRecorder.reflections, firstStart, object, producer,
            receiverReliable, firstReliable, receiverBestEffort, firstBestEffort,
            firstTag, true, "first mixed-rate passel");

         int secondStart = receiverRecorder.reflections.size();
         send(publisher, object, publisherReliable, secondReliable,
            publisherBestEffort, secondBestEffort, secondTag);
         await(receiver, callbackModel, () -> count(receiverRecorder.reflections,
            reliableHandle) >= 2, "second reliable attribute reflection");
         JavaTckSupport.check(receiverRecorder.reflections.size() > secondStart,
            "second update produced no reflection callback");
         JavaTckSupport.check(count(receiverRecorder.reflections, receiverBestEffort) == 1,
            "named-rate best-effort attribute was delivered twice");
         assertPassel(receiver, receiverRecorder.reflections, secondStart, object, producer,
            receiverReliable, secondReliable, receiverBestEffort, secondBestEffort,
            secondTag, false, "second mixed-rate passel");
      } finally {
         if (receiverJoined && subscribed) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass,
               JavaTckSupport.attributeSet(receiver, receiverReliable, receiverBestEffort)); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published) {
            try { publisher.unpublishObjectClassAttributes(publisherClass,
               JavaTckSupport.attributeSet(publisher, publisherReliable, publisherBestEffort)); }
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
         AttributeHandle reliable, byte[] reliableValue, AttributeHandle bestEffort,
         byte[] bestEffortValue, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
      values.put(reliable, reliableValue.clone());
      values.put(bestEffort, bestEffortValue.clone());
      publisher.updateAttributeValues(object, values, tag.clone());
   }

   private static int count(List<Reflection> reflections, AttributeHandle handle) {
      int result = 0;
      for (Reflection reflection : reflections) {
         if (reflection.values.containsKey(handle)) result++;
      }
      return result;
   }

   private static void assertPassel(RTIambassador receiver, List<Reflection> reflections,
         int start, ObjectInstanceHandle object, FederateHandle producer,
         AttributeHandle reliable, byte[] reliableValue, AttributeHandle bestEffort,
         byte[] bestEffortValue, byte[] tag, boolean expectBestEffort, String description)
         throws Exception {
      JavaTckSupport.check(start <= reflections.size(), description + " start was out of range");
      int reliableCount = 0;
      int bestEffortCount = 0;
      for (int i = start; i < reflections.size(); ++i) {
         Reflection reflection = reflections.get(i);
         JavaTckSupport.check(reflection.object.equals(object)
               && Arrays.equals(reflection.tag, tag)
               && producer.equals(reflection.producer)
               && (reflection.regions == null || reflection.regions.isEmpty())
               && reflection.transportation != null,
            description + " returned non-standard reflection metadata");
         String transportation = receiver.getTransportationTypeName(reflection.transportation);
         for (AttributeHandle attribute : reflection.values.keySet()) {
            byte[] value = reflection.values.get(attribute);
            if (reliable.equals(attribute)) {
               reliableCount++;
               JavaTckSupport.check(Arrays.equals(reliableValue, value)
                     && "HLAreliable".equals(transportation),
                  description + " returned the wrong reliable attribute value or transport");
            } else if (bestEffort.equals(attribute)) {
               bestEffortCount++;
               JavaTckSupport.check(expectBestEffort && Arrays.equals(bestEffortValue, value)
                     && "HLAbestEffort".equals(transportation),
                  description + " returned an unexpected best-effort value or transport");
            } else {
               throw new AssertionError(description + " returned an unexpected attribute");
            }
         }
      }
      JavaTckSupport.check(reliableCount == 1
            && bestEffortCount == (expectBestEffort ? 1 : 0),
         description + " did not deliver exactly the expected attributes");
   }

   private static void await(RTIambassador receiver, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(receiver);
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
      private final RegionHandleSet regions;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK mixed update-rate callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }

   private static final class EmptyFederateAmbassador {
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK publisher callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
