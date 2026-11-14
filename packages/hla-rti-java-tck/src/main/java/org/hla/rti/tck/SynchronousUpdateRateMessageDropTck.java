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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.locks.LockSupport;
import java.util.function.BooleanSupplier;

/** Measures synchronous update-rate reduction of ordinary best-effort updates. */
final class SynchronousUpdateRateMessageDropTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final double NANOS_PER_SECOND = 1_000_000_000.0;

   private SynchronousUpdateRateMessageDropTck() { }

   static void run(RtiFactory factory, String rateFom, String timeImplementation,
         String objectClassName, String attributeName, String updateRateDesignator,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "synchronous update-rate message dropping requires HLA_EVOKED");

      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-synchronous-update-rate-drop-" + UUID.randomUUID();
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
         publisher.joinFederationExecution("java-tck-update-rate-drop-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-update-rate-drop-receiver", federation);
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

         double subscribedRate = receiver.getUpdateRateValue(updateRateDesignator);
         JavaTckSupport.check(Double.isFinite(subscribedRate) && subscribedRate >= 0.25,
            "adapter rate must be finite and at least 0.25 updates per second");
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null, "registration returned a null object handle");
         await(receiver, () -> receiverRecorder.discoveries.contains(object),
            "object discovery");
         JavaTckSupport.check(receiver.getUpdateRateValueForAttribute(object, receiverAttribute)
               == subscribedRate,
            "named subscription did not select the queried update rate");

         long publisherIntervalNanos = (long) (NANOS_PER_SECOND / (3.0 * subscribedRate));
         long evokeIntervalNanos = (long) (NANOS_PER_SECOND / (4.0 * subscribedRate));
         JavaTckSupport.check(publisherIntervalNanos > 0L && evokeIntervalNanos > 0L,
            "adapter rate exceeds monotonic clock pacing resolution");

         double windowSeconds = Math.max(2.0, 10.0 / subscribedRate);
         long windowNanos = (long) (windowSeconds * NANOS_PER_SECOND);
         long measurementStart = System.nanoTime();
         long measurementEnd = measurementStart + windowNanos;
         long nextUpdate = measurementStart;
         long nextEvoke = measurementStart;
         int sentCount = 0;
         int evokeCount = 0;
         while (System.nanoTime() < measurementEnd) {
            long now = System.nanoTime();
            if (now >= nextEvoke) {
               receiver.evokeCallback(0.0);
               ++evokeCount;
               nextEvoke += evokeIntervalNanos;
               if (nextEvoke <= now) nextEvoke = now + evokeIntervalNanos;
            }
            if (now >= nextUpdate) {
               send(publisher, object, publisherAttribute, sentCount);
               ++sentCount;
               nextUpdate += publisherIntervalNanos;
               if (nextUpdate <= now) nextUpdate = now + publisherIntervalNanos;
            }
            LockSupport.parkNanos(500_000L);
         }

         double elapsedSeconds = (double) (measurementEnd - measurementStart)
            / NANOS_PER_SECOND;
         JavaTckSupport.check(sentCount > 2 && evokeCount > 2,
            "rate measurement did not produce enough paced updates and callback calls");
         double publisherRate = (sentCount - 1.0) / elapsedSeconds;
         double evokeRate = (evokeCount - 1.0) / elapsedSeconds;
         JavaTckSupport.check(publisherRate > subscribedRate && evokeRate > subscribedRate,
            "rate measurement did not establish Rp > Rs and Re > Rs");

         long drainDeadline = System.nanoTime()
            + (long) (NANOS_PER_SECOND / subscribedRate);
         while (System.nanoTime() < drainDeadline) {
            receiver.evokeCallback(0.0);
            LockSupport.parkNanos(500_000L);
         }

         boolean[] seen = new boolean[sentCount];
         int deliveredDuringWindow = 0;
         for (Reflection reflection : receiverRecorder.reflections) {
            JavaTckSupport.check(reflection.object.equals(object)
                  && reflection.values.size() == 1
                  && reflection.values.containsKey(receiverAttribute)
                  && reflection.receiveOrder,
               "rate reduction received an unexpected object, attribute, or order type");
            byte[] bytes = reflection.values.get(receiverAttribute);
            JavaTckSupport.check(bytes != null && bytes.length == 4,
               "rate reduction received malformed sequence evidence");
            long sequence = ((bytes[0] & 0xFFL) << 24)
               | ((bytes[1] & 0xFFL) << 16)
               | ((bytes[2] & 0xFFL) << 8)
               | (bytes[3] & 0xFFL);
            JavaTckSupport.check(sequence < sentCount && !seen[(int) sequence],
               "rate reduction duplicated or invented an update");
            seen[(int) sequence] = true;
            if (reflection.callbackEntryNanos <= measurementEnd) ++deliveredDuringWindow;
         }
         JavaTckSupport.check(deliveredDuringWindow <= sentCount,
            "rate reduction delivered more updates than were sent");

         double expectedDropCount = elapsedSeconds * (publisherRate - subscribedRate);
         double observedDropCount = sentCount - deliveredDuringWindow;
         String dropRateDiagnostic = "observed drops did not match (Rp-Rs) times the "
            + "measurement window within two boundary updates: expected=" + expectedDropCount
            + ", observed=" + observedDropCount + ", sent=" + sentCount
            + ", deliveredDuringWindow=" + deliveredDuringWindow + ", Rp=" + publisherRate
            + ", Re=" + evokeRate + ", Rs=" + subscribedRate
            + ", elapsedSeconds=" + elapsedSeconds;
         JavaTckSupport.check(Math.abs(observedDropCount - expectedDropCount) <= 2.0,
            dropRateDiagnostic);
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
         AttributeHandle attribute, int sequence) throws Exception {
      byte[] value = new byte[] {
         (byte) ((sequence >>> 24) & 0xFF),
         (byte) ((sequence >>> 16) & 0xFF),
         (byte) ((sequence >>> 8) & 0xFF),
         (byte) (sequence & 0xFF)
      };
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value);
      publisher.updateAttributeValues(object, values, new byte[0]);
   }

   private static void await(RTIambassador receiver, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         receiver.evokeCallback(0.0);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final boolean receiveOrder;
      private final long callbackEntryNanos;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            long callbackEntryNanos) {
         this.object = object;
         this.values = values.clone();
         this.receiveOrder = true;
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
                     (AttributeHandleValueMap) arguments[1], System.nanoTime()));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK synchronous update-rate callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
