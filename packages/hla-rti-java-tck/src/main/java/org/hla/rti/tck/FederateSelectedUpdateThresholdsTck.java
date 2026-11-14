package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Checks updates sent at two different application-selected sample thresholds. */
final class FederateSelectedUpdateThresholdsTck {
   private static final int FAST_THRESHOLD = 2;
   private static final int SLOW_THRESHOLD = 3;
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FederateSelectedUpdateThresholdsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String federateType, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "federate-selected update-threshold testing requires an adapter FOM");
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "federate-selected update-threshold testing requires an adapter logical-time name");
      JavaTckSupport.check(federateType != null && !federateType.isEmpty(),
         "federate-selected update-threshold testing requires a federate type");
      JavaTckSupport.check(objectClassName != null && !objectClassName.isEmpty(),
         "federate-selected update-threshold testing requires an object-class name");
      JavaTckSupport.check(attributeName != null && !attributeName.isEmpty(),
         "federate-selected update-threshold testing requires an attribute name");

      RTIambassador fastPublisher = factory.getRtiAmbassador();
      RTIambassador slowPublisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-selected-update-thresholds-" + UUID.randomUUID();
      boolean fastConnected = false;
      boolean slowConnected = false;
      boolean receiverConnected = false;
      boolean federationCreated = false;
      boolean fastJoined = false;
      boolean slowJoined = false;
      boolean receiverJoined = false;
      boolean fastPublished = false;
      boolean slowPublished = false;
      boolean receiverSubscribed = false;
      ObjectClassHandle fastClass = null;
      ObjectClassHandle slowClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandle fastAttribute = null;
      AttributeHandle slowAttribute = null;
      AttributeHandle receiverAttribute = null;
      AttributeHandleSet fastAttributes = null;
      AttributeHandleSet slowAttributes = null;
      AttributeHandleSet receiverAttributes = null;
      try {
         fastPublisher.connect(new NullFederateAmbassador(), callbackModel);
         fastConnected = true;
         slowPublisher.connect(new NullFederateAmbassador(), callbackModel);
         slowConnected = true;
         receiver.connect(recorder, callbackModel);
         receiverConnected = true;

         fastPublisher.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         FederateHandle fastFederate = fastPublisher.joinFederationExecution(
            "java-tck-threshold-fast-publisher", federateType, federation);
         fastJoined = true;
         FederateHandle slowFederate = slowPublisher.joinFederationExecution(
            "java-tck-threshold-slow-publisher", federateType, federation);
         slowJoined = true;
         receiver.joinFederationExecution(
            "java-tck-threshold-receiver", federateType, federation);
         receiverJoined = true;

         fastClass = fastPublisher.getObjectClassHandle(objectClassName);
         slowClass = slowPublisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         fastAttribute = fastPublisher.getAttributeHandle(fastClass, attributeName);
         slowAttribute = slowPublisher.getAttributeHandle(slowClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(fastClass != null && fastClass.equals(slowClass)
               && fastClass.equals(receiverClass)
               && fastAttribute != null && fastAttribute.equals(slowAttribute)
               && fastAttribute.equals(receiverAttribute),
            "federate-selected update-threshold lookup returned inconsistent handles");

         fastAttributes = fastPublisher.getAttributeHandleSetFactory().create();
         fastAttributes.add(fastAttribute);
         slowAttributes = slowPublisher.getAttributeHandleSetFactory().create();
         slowAttributes.add(slowAttribute);
         receiverAttributes = receiver.getAttributeHandleSetFactory().create();
         receiverAttributes.add(receiverAttribute);
         fastPublisher.publishObjectClassAttributes(fastClass, fastAttributes);
         fastPublished = true;
         slowPublisher.publishObjectClassAttributes(slowClass, slowAttributes);
         slowPublished = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         receiverSubscribed = true;

         ObjectInstanceHandle fastObject = fastPublisher.registerObjectInstance(fastClass);
         ObjectInstanceHandle slowObject = slowPublisher.registerObjectInstance(slowClass);
         JavaTckSupport.check(fastObject != null && slowObject != null
               && !fastObject.equals(slowObject),
            "federate-selected update-threshold registration returned invalid or duplicate objects");
         await(receiver, recorder, callbackModel,
            () -> recorder.hasDiscovery(fastObject) && recorder.hasDiscovery(slowObject),
            "federate-selected update-threshold object discovery");
         checkDiscoveries(recorder, fastObject, fastClass, fastFederate,
            slowObject, slowClass, slowFederate);

         for (int sample = 1; sample <= 6; ++sample) {
            if (sample % FAST_THRESHOLD == 0) {
               sendAtThreshold(fastPublisher, fastObject, fastAttribute,
                  FAST_THRESHOLD, sample);
            }
            if (sample % SLOW_THRESHOLD == 0) {
               sendAtThreshold(slowPublisher, slowObject, slowAttribute,
                  SLOW_THRESHOLD, sample);
            }
         }

         await(receiver, recorder, callbackModel,
            () -> recorder.reflections().size() >= 5,
            "federate-selected threshold update reflections");
         checkReflections(recorder, receiverAttribute, fastObject, fastFederate,
            slowObject, slowFederate);

         receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
         receiverSubscribed = false;
         fastPublisher.unpublishObjectClassAttributes(fastClass, fastAttributes);
         fastPublished = false;
         slowPublisher.unpublishObjectClassAttributes(slowClass, slowAttributes);
         slowPublished = false;
         fastPublisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         fastJoined = false;
         slowPublisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         slowJoined = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         fastPublisher.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (receiverSubscribed && receiverClass != null && receiverAttributes != null) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (fastPublished && fastClass != null && fastAttributes != null) {
            try { fastPublisher.unpublishObjectClassAttributes(fastClass, fastAttributes); }
            catch (Exception ignored) { }
         }
         if (slowPublished && slowClass != null && slowAttributes != null) {
            try { slowPublisher.unpublishObjectClassAttributes(slowClass, slowAttributes); }
            catch (Exception ignored) { }
         }
         if (fastJoined) resign(fastPublisher, ResignAction.DELETE_OBJECTS);
         if (slowJoined) resign(slowPublisher, ResignAction.DELETE_OBJECTS);
         if (receiverJoined) resign(receiver, ResignAction.NO_ACTION);
         if (federationCreated) {
            try { fastPublisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (slowConnected) disconnect(slowPublisher);
         if (fastConnected) disconnect(fastPublisher);
      }
   }

   private static void sendAtThreshold(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle attribute, int threshold, int sample) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, new byte[] {(byte) sample});
      publisher.updateAttributeValues(object, values,
         new byte[] {(byte) threshold, (byte) sample});
   }

   private static void await(RTIambassador receiver, Recorder recorder,
         CallbackModel callbackModel, BooleanSupplier complete, String description)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline) {
         if (complete.getAsBoolean()) return;
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            receiver.evokeCallback(0.01);
         } else {
            Thread.sleep(1);
         }
      }
      throw new AssertionError("Timed out waiting for " + description);
   }

   private static void checkDiscoveries(Recorder recorder,
         ObjectInstanceHandle fastObject, ObjectClassHandle fastClass,
         FederateHandle fastFederate, ObjectInstanceHandle slowObject,
         ObjectClassHandle slowClass, FederateHandle slowFederate) {
      List<Discovery> discoveries = recorder.discoveries();
      JavaTckSupport.check(discoveries.size() == 2,
         "federate-selected update-threshold test expected exactly two discoveries");
      boolean foundFast = false;
      boolean foundSlow = false;
      for (Discovery discovery : discoveries) {
         if (fastObject.equals(discovery.object)) {
            JavaTckSupport.check(fastClass.equals(discovery.objectClass)
                  && fastFederate.equals(discovery.producer),
               "fast-threshold discovery reported incorrect class or producer identity");
            foundFast = true;
         } else if (slowObject.equals(discovery.object)) {
            JavaTckSupport.check(slowClass.equals(discovery.objectClass)
                  && slowFederate.equals(discovery.producer),
               "slow-threshold discovery reported incorrect class or producer identity");
            foundSlow = true;
         } else {
            throw new AssertionError("federate-selected threshold test discovered an unknown object");
         }
      }
      JavaTckSupport.check(foundFast && foundSlow,
         "federate-selected update-threshold discovery omitted a publisher object");
   }

   private static void checkReflections(Recorder recorder, AttributeHandle attribute,
         ObjectInstanceHandle fastObject, FederateHandle fastFederate,
         ObjectInstanceHandle slowObject, FederateHandle slowFederate) {
      List<Reflection> reflections = recorder.reflections();
      JavaTckSupport.check(reflections.size() == 5,
         "federate-selected update-threshold test expected exactly five reflections");
      List<Integer> fastSamples = new ArrayList<>();
      List<Integer> slowSamples = new ArrayList<>();
      for (Reflection reflection : reflections) {
         JavaTckSupport.check(reflection.values.size() == 1
               && reflection.values.containsKey(attribute)
               && (reflection.regions == null || reflection.regions.isEmpty()),
            "federate-selected threshold reflection returned unexpected attribute metadata");
         byte[] value = reflection.values.get(attribute);
         JavaTckSupport.check(value != null && value.length == 1,
            "federate-selected threshold reflection returned a malformed sample value");
         int sample = Byte.toUnsignedInt(value[0]);
         if (fastObject.equals(reflection.object)) {
            JavaTckSupport.check(fastFederate.equals(reflection.producer),
               "fast-threshold reflection identified the wrong producer");
            JavaTckSupport.check(Arrays.equals(reflection.tag,
                  new byte[] {(byte) FAST_THRESHOLD, (byte) sample}),
               "fast-threshold reflection returned the wrong threshold/sample tag");
            fastSamples.add(sample);
         } else if (slowObject.equals(reflection.object)) {
            JavaTckSupport.check(slowFederate.equals(reflection.producer),
               "slow-threshold reflection identified the wrong producer");
            JavaTckSupport.check(Arrays.equals(reflection.tag,
                  new byte[] {(byte) SLOW_THRESHOLD, (byte) sample}),
               "slow-threshold reflection returned the wrong threshold/sample tag");
            slowSamples.add(sample);
         } else {
            throw new AssertionError("federate-selected threshold test received an unknown object");
         }
      }
      JavaTckSupport.check(fastSamples.equals(List.of(2, 4, 6))
            && slowSamples.equals(List.of(3, 6)),
         "reflections did not match the two federates' distinct threshold-selected updates");
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Recorder extends NullFederateAmbassador {
      private final List<Discovery> discoveries = new ArrayList<>();
      private final List<Reflection> reflections = new ArrayList<>();

      @Override
      public synchronized void discoverObjectInstance(ObjectInstanceHandle object,
            ObjectClassHandle objectClass, String objectName, FederateHandle producingFederate) {
         discoveries.add(new Discovery(object, objectClass, producingFederate));
         notifyAll();
      }

      @Override
      public synchronized void reflectAttributeValues(ObjectInstanceHandle object,
            AttributeHandleValueMap attributeValues, byte[] userSuppliedTag,
            TransportationTypeHandle transportation, FederateHandle producingFederate,
            RegionHandleSet sentRegions) {
         reflections.add(new Reflection(object, copyValues(attributeValues),
            userSuppliedTag == null ? new byte[0] : userSuppliedTag.clone(),
            producingFederate, sentRegions));
         notifyAll();
      }

      synchronized boolean hasDiscovery(ObjectInstanceHandle object) {
         for (Discovery discovery : discoveries) {
            if (object.equals(discovery.object)) return true;
         }
         return false;
      }

      synchronized List<Discovery> discoveries() {
         return new ArrayList<>(discoveries);
      }

      synchronized List<Reflection> reflections() {
         return new ArrayList<>(reflections);
      }

      private static java.util.Map<AttributeHandle, byte[]> copyValues(
            AttributeHandleValueMap values) {
         java.util.Map<AttributeHandle, byte[]> copy = new java.util.HashMap<>();
         for (java.util.Map.Entry<AttributeHandle, byte[]> entry : values.entrySet()) {
            copy.put(entry.getKey(), entry.getValue().clone());
         }
         return copy;
      }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final FederateHandle producer;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            FederateHandle producer) {
         this.object = object;
         this.objectClass = objectClass;
         this.producer = producer;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final java.util.Map<AttributeHandle, byte[]> values;
      private final byte[] tag;
      private final FederateHandle producer;
      private final RegionHandleSet regions;

      private Reflection(ObjectInstanceHandle object,
            java.util.Map<AttributeHandle, byte[]> values, byte[] tag,
            FederateHandle producer, RegionHandleSet regions) {
         this.object = object;
         this.values = values;
         this.tag = tag;
         this.producer = producer;
         this.regions = regions;
      }
   }
}
