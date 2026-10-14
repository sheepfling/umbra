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
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Re-evaluates direct regional update relevance when a callback is serviced. */
final class RegionalAttributeUpdateCallbackDdmRecheckTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private RegionalAttributeUpdateCallbackDdmRecheckTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador active = factory.getRtiAmbassador();
      RTIambassador rechecked = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder activeCallbacks = new Recorder();
      Recorder recheckedCallbacks = new Recorder();
      String federation = "java-tck-regional-callback-ddm-recheck-" + UUID.randomUUID();
      boolean publisherConnected = false, activeConnected = false, recheckedConnected = false;
      boolean created = false, publisherJoined = false, activeJoined = false, recheckedJoined = false;
      boolean published = false, activeSubscribed = false, recheckedSubscribed = false;
      ObjectClassHandle publisherClass = null, activeClass = null, recheckedClass = null;
      AttributeHandle publisherAttribute = null, activeAttribute = null, recheckedAttribute = null;
      AttributeHandleSet publisherAttributes = null, activeAttributes = null, recheckedAttributes = null;
      AttributeSetRegionSetPairList publisherPair = null, activePair = null, recheckedPair = null;
      RegionHandle sourceRegion = null, activeRegion = null, recheckedRegion = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         active.connect(activeCallbacks.proxy(), callbackModel);
         activeConnected = true;
         rechecked.connect(recheckedCallbacks.proxy(), callbackModel);
         recheckedConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producingFederate = publisher.joinFederationExecution(
            "java-tck-regional-callback-publisher", federation);
         publisherJoined = true;
         active.joinFederationExecution("java-tck-regional-callback-active", federation);
         activeJoined = true;
         rechecked.joinFederationExecution("java-tck-regional-callback-rechecked", federation);
         recheckedJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         activeClass = active.getObjectClassHandle(objectClassName);
         recheckedClass = rechecked.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         activeAttribute = active.getAttributeHandle(activeClass, attributeName);
         recheckedAttribute = rechecked.getAttributeHandle(recheckedClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle activeFirst = active.getDimensionHandle(firstDimensionName);
         DimensionHandle activeSecond = active.getDimensionHandle(secondDimensionName);
         DimensionHandle recheckedFirst = rechecked.getDimensionHandle(firstDimensionName);
         DimensionHandle recheckedSecond = rechecked.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherClass.equals(activeClass)
               && publisherClass.equals(recheckedClass)
               && publisherAttribute.equals(activeAttribute)
               && publisherAttribute.equals(recheckedAttribute)
               && publisherFirst.equals(activeFirst) && publisherSecond.equals(activeSecond)
               && publisherFirst.equals(recheckedFirst) && publisherSecond.equals(recheckedSecond),
            "adapter DDM FOM returned inconsistent object, attribute, or dimension handles");
         checkDimensions(publisher, publisherClass, publisherFirst, publisherSecond);

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         activeAttributes = JavaTckSupport.attributeSet(active, activeAttribute);
         recheckedAttributes = JavaTckSupport.attributeSet(rechecked, recheckedAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5);
         activeRegion = createRegion(active, activeFirst, activeSecond, 2, 6);
         recheckedRegion = createRegion(rechecked, recheckedFirst, recheckedSecond, 2, 6);
         publisherPair = association(publisher, publisherAttributes, sourceRegion);
         activePair = association(active, activeAttributes, activeRegion);
         recheckedPair = association(rechecked, recheckedAttributes, recheckedRegion);
         active.setConveyRegionDesignatorSetsSwitch(true);
         rechecked.setConveyRegionDesignatorSetsSwitch(true);
         active.subscribeObjectClassAttributesWithRegions(activeClass, activePair);
         activeSubscribed = true;
         rechecked.subscribeObjectClassAttributesWithRegions(recheckedClass, recheckedPair);
         recheckedSubscribed = true;
         object = publisher.registerObjectInstanceWithRegions(publisherClass, publisherPair);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional callback DDM registration returned an invalid object handle");
         ObjectInstanceHandle registeredObject = object;
         await(publisher, active, rechecked, callbackModel,
            () -> activeCallbacks.discoveries.contains(registeredObject)
               && recheckedCallbacks.discoveries.contains(registeredObject),
            "regional object discovery");

         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] firstValue = new byte[] {0x41, 0x31};
         byte[] firstTag = new byte[] {0x52, 0x31};
         update(publisher, object, publisherAttribute, firstValue, firstTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(activeCallbacks.reflections.isEmpty()
                  && recheckedCallbacks.reflections.isEmpty(),
               "evoked regional update arrived before callback servicing");
            moveRegion(rechecked, recheckedRegion, recheckedFirst, recheckedSecond, false);
            await(publisher, active, rechecked, callbackModel,
               () -> activeCallbacks.reflections.size() >= 1,
               "active receiver reflection after region mutation");
            for (int pass = 0; pass < 8; ++pass) pump(rechecked, callbackModel);
            JavaTckSupport.check(recheckedCallbacks.reflections.isEmpty(),
               "evoked regional callback delivered a stale disjoint reflection");
         } else {
            await(publisher, active, rechecked, callbackModel,
               () -> activeCallbacks.reflections.size() >= 1
                  && recheckedCallbacks.reflections.size() >= 1,
               "immediate regional reflections before region mutation");
            moveRegion(rechecked, recheckedRegion, recheckedFirst, recheckedSecond, false);
         }
         requireReflection(active, activeCallbacks.reflections, object, activeAttribute,
            firstValue, firstTag, producingFederate, sourceRegion,
            "active receiver's first regional reflection");
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            requireReflection(rechecked, recheckedCallbacks.reflections, object, recheckedAttribute,
               firstValue, firstTag, producingFederate, sourceRegion,
               "immediate rechecked receiver's first reflection");
         }
         JavaTckSupport.check(publisherCallbacks.reflections.isEmpty(),
            "regional attribute callback DDM update looped a reflection to the publisher");

         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] secondValue = new byte[] {0x41, 0x32};
         byte[] secondTag = new byte[] {0x52, 0x32};
         update(publisher, object, publisherAttribute, secondValue, secondTag);
         await(publisher, active, rechecked, callbackModel,
            () -> activeCallbacks.reflections.size() >= 1,
            "active receiver reflection while other region is disjoint");
         for (int pass = 0; pass < 8; ++pass) pump(rechecked, callbackModel);
         JavaTckSupport.check(recheckedCallbacks.reflections.isEmpty(),
            "regional callback delivered while the receiver region was disjoint");
         requireReflection(active, activeCallbacks.reflections, object, activeAttribute,
            secondValue, secondTag, producingFederate, sourceRegion,
            "active receiver's second regional reflection");

         moveRegion(rechecked, recheckedRegion, recheckedFirst, recheckedSecond, true);
         activeCallbacks.reflections.clear();
         recheckedCallbacks.reflections.clear();
         byte[] restoredValue = new byte[] {0x41, 0x33};
         byte[] restoredTag = new byte[] {0x52, 0x33};
         update(publisher, object, publisherAttribute, restoredValue, restoredTag);
         await(publisher, active, rechecked, callbackModel,
            () -> activeCallbacks.reflections.size() >= 1
               && recheckedCallbacks.reflections.size() >= 1,
            "regional reflection after restoring overlap");
         requireReflection(active, activeCallbacks.reflections, object, activeAttribute,
            restoredValue, restoredTag, producingFederate, sourceRegion,
            "active receiver's restored-overlap reflection");
         requireReflection(rechecked, recheckedCallbacks.reflections, object, recheckedAttribute,
            restoredValue, restoredTag, producingFederate, sourceRegion,
            "rechecked receiver's restored-overlap reflection");

         active.unsubscribeObjectClassAttributesWithRegions(activeClass, activePair);
         activeSubscribed = false;
         rechecked.unsubscribeObjectClassAttributesWithRegions(recheckedClass, recheckedPair);
         recheckedSubscribed = false;
         publisher.unassociateRegionsForUpdates(object, publisherPair);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         active.deleteRegion(activeRegion); activeRegion = null;
         rechecked.deleteRegion(recheckedRegion); recheckedRegion = null;
         publisher.deleteRegion(sourceRegion); sourceRegion = null;
         active.resignFederationExecution(ResignAction.NO_ACTION);
         activeJoined = false;
         rechecked.resignFederationExecution(ResignAction.NO_ACTION);
         recheckedJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (activeSubscribed && activeJoined && activePair != null) try {
            active.unsubscribeObjectClassAttributesWithRegions(activeClass, activePair);
         } catch (Exception ignored) { }
         if (recheckedSubscribed && recheckedJoined && recheckedPair != null) try {
            rechecked.unsubscribeObjectClassAttributesWithRegions(recheckedClass, recheckedPair);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         deleteRegion(active, activeRegion, activeJoined);
         deleteRegion(rechecked, recheckedRegion, recheckedJoined);
         deleteRegion(publisher, sourceRegion, publisherJoined);
         if (activeJoined) try { active.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (recheckedJoined) try { rechecked.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (activeConnected) try { active.disconnect(); } catch (Exception ignored) { }
         if (recheckedConnected) try { rechecked.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      java.util.Set<DimensionHandle> dimensions =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(dimensions.contains(first) && dimensions.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         "adapter FOM does not expose the dimensions and bounds required by regional DDM recheck");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions));
      return result;
   }

   private static void moveRegion(RTIambassador ambassador, RegionHandle region,
         DimensionHandle first, DimensionHandle second, boolean overlap) throws Exception {
      long lower = overlap ? 2 : 7;
      long upper = overlap ? 6 : 9;
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
   }

   private static void update(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      publisher.updateAttributeValues(object, values, tag.clone());
   }

   private static void requireReflection(RTIambassador receiver, List<Reflection> reflections,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value, byte[] tag,
         FederateHandle producer, RegionHandle sourceRegion, String description) throws Exception {
      JavaTckSupport.check(reflections.size() == 1,
         description + " produced the wrong reflection count");
      Reflection reflection = reflections.get(0);
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer),
         description + " returned the wrong object, value, tag, or producing federate");
      TransportationTypeHandle reliable = receiver.getTransportationTypeHandle("HLAreliable");
      JavaTckSupport.check(reliable.equals(reflection.transportation),
         description + " returned a transportation type other than HLAreliable");
      JavaTckSupport.check(reflection.regions != null && reflection.regions.size() == 1
            && reflection.regions.contains(sourceRegion),
         description + " omitted the source-region designator");
   }

   private static void await(RTIambassador first, RTIambassador second, RTIambassador third,
         CallbackModel model, java.util.function.BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         pump(second, model);
         pump(third, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean joined) {
      if (joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private RegionHandleSet regions;
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  reflection.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflection.tag = ((byte[]) arguments[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) arguments[3];
                  reflection.producer = (FederateHandle) arguments[4];
                  reflection.regions = (RegionHandleSet) arguments[5];
                  reflections.add(reflection);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional callback DDM recheck";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
