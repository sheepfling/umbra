package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies regional object delivery requires overlap in every declared dimension. */
final class RegionalThreeDimensionalOverlapTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private RegionalThreeDimensionalOverlapTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, String thirdDimensionName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied three-dimensional FOM is required");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-regional-three-dimensional-overlap-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      RegionHandle sourceRegion = null;
      RegionHandle subscriberRegion = null;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle subscriberClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandle subscriberAttribute = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeSetRegionSetPairList sourcePairs = null;
      AttributeSetRegionSetPairList subscriberPairs = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle publisherFederate =
            publisher.joinFederationExecution("three-dimensional-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("three-dimensional-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         DimensionHandle[] publisherDimensions = new DimensionHandle[] {
            publisher.getDimensionHandle(firstDimensionName),
            publisher.getDimensionHandle(secondDimensionName),
            publisher.getDimensionHandle(thirdDimensionName)
         };
         DimensionHandle[] subscriberDimensions = new DimensionHandle[] {
            subscriber.getDimensionHandle(firstDimensionName),
            subscriber.getDimensionHandle(secondDimensionName),
            subscriber.getDimensionHandle(thirdDimensionName)
         };
         JavaTckSupport.check(publisherClass.equals(subscriberClass)
               && publisherAttribute.equals(subscriberAttribute),
            "adapter three-dimensional FOM resolved inconsistent class or attribute handles");
         DimensionHandleSet available =
            publisher.getAvailableDimensionsForObjectClass(publisherClass);
         DimensionHandleSet expectedDimensions =
            publisher.getDimensionHandleSetFactory().create();
         for (int index = 0; index < publisherDimensions.length; index++) {
            expectedDimensions.add(publisherDimensions[index]);
            JavaTckSupport.check(publisher.getDimensionName(publisherDimensions[index])
                  .equals(index == 0 ? firstDimensionName
                     : index == 1 ? secondDimensionName : thirdDimensionName)
                  && !publisherDimensions[index].equals(publisherDimensions[(index + 1) % 3])
                  && publisherDimensions[index].equals(subscriberDimensions[index])
                  && publisher.getDimensionUpperBound(publisherDimensions[index]) >= 8,
               "three-dimensional FOM resolved an unexpected name, duplicate, or insufficient range");
         }
         JavaTckSupport.check(available.equals(expectedDimensions),
            "three-dimensional object class did not expose exactly the adapter dimensions");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         sourceRegion = createRegion(publisher, publisherDimensions, 0, 5, 0, 2, 0, 1);
         subscriberRegion = createRegion(subscriber, subscriberDimensions, 0, 5, 0, 2, 0, 1);
         subscriber.setConveyRegionDesignatorSetsSwitch(true);
         sourcePairs = pairList(publisher, publisherAttribute, sourceRegion);
         subscriberPairs = pairList(subscriber, subscriberAttribute, subscriberRegion);
         subscriber.subscribeObjectClassAttributesWithRegions(subscriberClass, subscriberPairs);
         subscribed = true;
         object = publisher.registerObjectInstanceWithRegions(publisherClass, sourcePairs);
         JavaTckSupport.check(object != null,
            "three-dimensional regional registration returned a null object handle");
         ObjectInstanceHandle registeredObject = object;
         await(subscriber, callbackModel,
            () -> !subscriberCallbacks.discoveries.isEmpty(),
            "three-dimensional regional discovery");
         JavaTckSupport.check(registeredObject.equals(subscriberCallbacks.discoveries.get(0).object)
               && subscriberClass.equals(subscriberCallbacks.discoveries.get(0).objectClass),
            "three-dimensional regional discovery returned the wrong object or class");

         FederateHandle producer = publisherFederate;
         update(publisher, object, publisherAttribute, new byte[] {0x10}, new byte[] {0x49, 0x31});
         await(subscriber, callbackModel,
            () -> !subscriberCallbacks.reflections.isEmpty(),
            "three-dimensional regional initial reflection");
         assertReflection(subscriberCallbacks.reflections.get(0), object, subscriberAttribute,
            producer, new byte[] {0x10}, new byte[] {0x49, 0x31}, sourceRegion, subscriber);
         int initialReflectionCount = subscriberCallbacks.reflections.size();

         verifyDimensionFilter(publisher, subscriber, callbackModel, object, publisherAttribute,
            subscriberCallbacks, sourceRegion, publisherDimensions[0], new RangeBounds(6, 8), new RangeBounds(0, 5),
            initialReflectionCount, "first");
         verifyDimensionFilter(publisher, subscriber, callbackModel, object, publisherAttribute,
            subscriberCallbacks, sourceRegion, publisherDimensions[1], new RangeBounds(3, 4), new RangeBounds(0, 2),
            initialReflectionCount, "second");
         verifyDimensionFilter(publisher, subscriber, callbackModel, object, publisherAttribute,
            subscriberCallbacks, sourceRegion, publisherDimensions[2], new RangeBounds(2, 3), new RangeBounds(0, 1),
            initialReflectionCount, "third");

         update(publisher, object, publisherAttribute, new byte[] {0x30}, new byte[] {0x52, 0x31});
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.reflections.size() > initialReflectionCount,
            "three-dimensional regional restored reflection");
         assertReflection(subscriberCallbacks.reflections.get(initialReflectionCount), object,
            subscriberAttribute, producer, new byte[] {0x30}, new byte[] {0x52, 0x31},
            sourceRegion, subscriber);
      } finally {
         if (subscriberJoined && subscribed && subscriberClass != null && subscriberPairs != null) {
            try { subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberClass, subscriberPairs); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && object != null && sourcePairs != null) {
            try { publisher.unassociateRegionsForUpdates(object, sourcePairs); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherClass != null && publisherAttributes != null) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined && subscriberRegion != null) {
            try { subscriber.deleteRegion(subscriberRegion); } catch (Exception ignored) { }
         }
         if (publisherJoined && sourceRegion != null) {
            try { publisher.deleteRegion(sourceRegion); } catch (Exception ignored) { }
         }
         if (subscriberJoined) resign(subscriber, ResignAction.NO_ACTION);
         if (publisherJoined) resign(publisher, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle[] dimensions,
         long firstLower, long firstUpper, long secondLower, long secondUpper,
         long thirdLower, long thirdUpper) throws Exception {
      DimensionHandleSet set = ambassador.getDimensionHandleSetFactory().create();
      for (DimensionHandle dimension : dimensions) set.add(dimension);
      RegionHandle region = ambassador.createRegion(set);
      ambassador.setRangeBounds(region, dimensions[0], new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, dimensions[1], new RangeBounds(secondLower, secondUpper));
      ambassador.setRangeBounds(region, dimensions[2], new RangeBounds(thirdLower, thirdUpper));
      commitRegion(ambassador, region);
      return region;
   }

   private static AttributeSetRegionSetPairList pairList(RTIambassador ambassador,
         AttributeHandle attribute, RegionHandle region) throws Exception {
      AttributeHandleSet attributes = JavaTckSupport.attributeSet(ambassador, attribute);
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      pairs.add(new AttributeRegionAssociation(attributes, regions));
      return pairs;
   }

   private static void commitRegion(RTIambassador ambassador, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
   }

   private static void verifyDimensionFilter(RTIambassador publisher, RTIambassador subscriber,
         CallbackModel callbackModel, ObjectInstanceHandle object, AttributeHandle attribute,
         Recorder callbackRecorder, RegionHandle sourceRegion, DimensionHandle dimension,
         RangeBounds disjoint, RangeBounds restored, int expectedReflectionCount,
         String description) throws Exception {
      publisher.setRangeBounds(sourceRegion, dimension, disjoint);
      commitRegion(publisher, sourceRegion);
      update(publisher, object, attribute, new byte[] {0x20}, new byte[] {0x53, 0x50});
      settle(subscriber, callbackModel);
      JavaTckSupport.check(callbackRecorder.reflections.size() == expectedReflectionCount,
         "three-dimensional regional " + description + "-dimension disjoint source was delivered");
      publisher.setRangeBounds(sourceRegion, dimension, restored);
      commitRegion(publisher, sourceRegion);
   }

   private static void update(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      publisher.updateAttributeValues(object, values, tag.clone());
   }

   private static void assertReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, FederateHandle producer, byte[] expectedValue, byte[] expectedTag,
         RegionHandle sourceRegion, RTIambassador receiver) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(expectedValue, reflection.values.get(attribute))
            && Arrays.equals(expectedTag, reflection.tag)
            && producer.equals(reflection.producer),
         "three-dimensional regional reflection returned the wrong value or producer metadata");
      JavaTckSupport.check(reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty()
            && reflection.regions != null && reflection.regions.size() == 1
            && reflection.regions.contains(sourceRegion),
         "three-dimensional regional reflection omitted transportation or source-region metadata");
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else {
         long deadline = System.nanoTime() + 100_000_000L;
         while (System.nanoTime() < deadline) Thread.yield();
      }
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private Discovery(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         objectClass = (ObjectClassHandle) arguments[1];
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap received = (AttributeHandleValueMap) arguments[1];
         received.forEach((handle, value) -> values.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         RegionHandleSet receivedRegions = (RegionHandleSet) arguments[5];
         regions = receivedRegions == null ? null : (RegionHandleSet) receivedRegions.clone();
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
                     && arguments.length == 4) discoveries.add(new Discovery(arguments));
               else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) reflections.add(new Reflection(arguments));
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK three-dimensional regional callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
