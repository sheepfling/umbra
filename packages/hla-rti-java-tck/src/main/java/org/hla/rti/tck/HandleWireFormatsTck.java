package org.hla.rti.tck;

import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;

/** Verifies standard Java handle encodings and public handle-factory decoders. */
final class HandleWireFormatsTck {
   private HandleWireFormatsTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-handle-wire-formats-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean constrained = false;
      boolean regulating = false;
      RegionHandle region = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-handle-wire-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-handle-wire-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute)
               && publisherFirst.equals(receiverFirst)
               && publisherSecond.equals(receiverSecond),
            "adapter FOM did not resolve stable object, attribute, and dimension handles");
         JavaTckSupport.check(publisher.getAvailableDimensionsForObjectClass(publisherClass)
               .contains(publisherFirst)
               && publisher.getAvailableDimensionsForObjectClass(publisherClass)
                  .contains(publisherSecond),
            "adapter object class does not expose both configured dimensions");

         DimensionHandleSet dimensions = publisher.getDimensionHandleSetFactory().create();
         dimensions.add(publisherFirst);
         dimensions.add(publisherSecond);
         region = publisher.createRegion(dimensions);
         RegionHandleSet regions = publisher.getRegionHandleSetFactory().create();
         regions.add(region);
         long firstUpper = publisher.getDimensionUpperBound(publisherFirst);
         long secondUpper = publisher.getDimensionUpperBound(publisherSecond);
         JavaTckSupport.check(firstUpper >= 5 && secondUpper >= 7,
            "adapter dimension bounds are too small for the handle-wire fixture");
         publisher.setRangeBounds(region, publisherFirst, new RangeBounds(1, 5));
         publisher.setRangeBounds(region, publisherSecond, new RangeBounds(2, 7));
         publisher.commitRegionModifications(regions);

         assertHandleRoundTrip(publisherFirst.encodedLength(), publisherFirst::encode,
            (bytes, offset) -> publisher.getDimensionHandleFactory().decode(bytes, offset),
            publisherFirst, "dimension handle");
         assertHandleRoundTrip(region.encodedLength(), region::encode,
            (bytes, offset) -> publisher.getRegionHandleFactory().decode(bytes, offset),
            region, "region handle");

         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet receiverAttributes =
            JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(publisher, receiver, callbackModel,
            () -> receiverRecorder.discoveredObject != null,
            "registered object discovery");
         JavaTckSupport.check(object.equals(receiverRecorder.discoveredObject),
            "receiver discovered a different object-instance handle");

         receiver.enableTimeConstrained();
         constrained = true;
         publisher.enableTimeRegulation(
            (LogicalTimeInterval<?>) publisher.getTimeFactory().makeEpsilon());
         regulating = true;
         await(publisher, receiver, callbackModel,
            () -> publisherRecorder.timeRegulationEnabled && receiverRecorder.timeConstrainedEnabled,
            "timestamped-role enable callbacks");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeInterval<?> epsilon = (LogicalTimeInterval<?>) timeFactory.makeEpsilon();
         LogicalTime<?, ?> messageTime = addSteps(publisher.queryLogicalTime(), epsilon, 3);
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, new byte[] {0x71, 0x72, 0x73});
         MessageRetractionReturn update = publisher.updateAttributeValues(
            object, values, new byte[] {0x48, 0x57, 0x46}, messageTime);
         JavaTckSupport.check(update != null && update.retractionHandleIsValid
               && update.handle != null,
            "timestamped attribute update did not return a valid retraction handle");
         MessageRetractionHandle retraction = update.handle;
         assertHandleRoundTrip(retraction.encodedLength(), retraction::encode,
            (bytes, offset) -> publisher.getMessageRetractionHandleFactory()
               .decode(bytes, offset), retraction, "message-retraction handle");

         expectEmptyDecodeRejected(
            (bytes, offset) -> publisher.getDimensionHandleFactory().decode(bytes, offset),
            "dimension");
         expectEmptyDecodeRejected(
            (bytes, offset) -> publisher.getRegionHandleFactory().decode(bytes, offset), "region");
         expectEmptyDecodeRejected(
            (bytes, offset) -> publisher.getMessageRetractionHandleFactory()
               .decode(bytes, offset), "message-retraction");

         publisher.retract(retraction);
         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         publisher.deleteRegion(region);
         region = null;
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try {
               receiver.unsubscribeObjectClassAttributes(
                  receiver.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(receiver,
                     receiver.getAttributeHandle(receiver.getObjectClassHandle(objectClassName),
                        attributeName)));
            } catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try {
               ObjectClassHandle objectClass = publisher.getObjectClassHandle(objectClassName);
               publisher.unpublishObjectClassAttributes(objectClass,
                  JavaTckSupport.attributeSet(publisher,
                     publisher.getAttributeHandle(objectClass, attributeName)));
            } catch (Exception ignored) { }
         }
         if (region != null && publisherJoined) {
            try { publisher.deleteRegion(region); } catch (Exception ignored) { }
         }
         if (receiverJoined) resign(receiver, ResignAction.NO_ACTION);
         if (publisherJoined) resign(publisher, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime addSteps(LogicalTime start,
         LogicalTimeInterval interval, int count) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < count; ++i) {
         result = result.add(interval);
      }
      return result;
   }

   private static void assertHandleRoundTrip(int encodedLength,
         JavaTckSupport.HandleEncoder encoder, HandleDecoder decoder,
         Object expected, String description) throws Exception {
      byte[] encoded = JavaTckSupport.encode(encodedLength, encoder);
      JavaTckSupport.check(encoded.length == encodedLength && encodedLength > 0,
         description + " reported an invalid encoded length");
      Object decoded = decoder.decode(encoded, 0);
      JavaTckSupport.check(expected.equals(decoded)
            && expected.hashCode() == decoded.hashCode(),
         description + " did not round-trip through its standard handle factory");
      JavaTckSupport.check(Arrays.equals(encoded,
            JavaTckSupport.encode(encodedLength, encoder)),
         description + " produced different bytes on repeated standard encoding");
   }

   private static void expectEmptyDecodeRejected(HandleDecoder decoder,
         String description) throws Exception {
      JavaTckSupport.expectFailure(() -> decoder.decode(new byte[0], 0),
         "CouldNotDecode", description + " handle factory accepted an empty encoding");
   }

   private static void await(RTIambassador publisher, RTIambassador receiver,
         CallbackModel callbackModel, java.util.function.BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            publisher.evokeCallback(0.0);
            receiver.evokeCallback(0.0);
         } else {
            Thread.sleep(2);
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private interface HandleDecoder {
      Object decode(byte[] bytes, int offset) throws Exception;
   }

   private static final class Recorder {
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK handle wire callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
