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
import hla.rti1516_2025.RangeBounds;
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

/** Verifies independent per-attribute regional filtering for multi-attribute updates. */
final class RegionalMultiAttributeUpdateTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private RegionalMultiAttributeUpdateTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String firstAttributeName, String secondAttributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied multi-attribute FOM is required");

      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador flavor = factory.getRtiAmbassador();
      RTIambassador organic = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder flavorCallbacks = new Recorder();
      Recorder organicCallbacks = new Recorder();
      String federation = "java-tck-regional-multi-attribute-update-" + UUID.randomUUID();
      boolean[] connected = new boolean[3];
      boolean created = false;
      boolean[] joined = new boolean[3];
      boolean published = false;
      boolean flavorSubscribed = false;
      boolean organicSubscribed = false;
      RegionHandle sourceFlavor = null;
      RegionHandle sourceOrganic = null;
      RegionHandle flavorRegion = null;
      RegionHandle organicRegion = null;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle flavorClass = null;
      ObjectClassHandle organicClass = null;
      AttributeHandle firstAttribute = null;
      AttributeHandle secondAttribute = null;
      AttributeHandleSet publishedAttributes = null;
      AttributeSetRegionSetPairList sourcePairs = null;
      AttributeSetRegionSetPairList flavorPairs = null;
      AttributeSetRegionSetPairList organicPairs = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         connected[0] = true;
         flavor.connect(flavorCallbacks.proxy(), callbackModel);
         connected[1] = true;
         organic.connect(organicCallbacks.proxy(), callbackModel);
         connected[2] = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle publisherFederate =
            publisher.joinFederationExecution("multi-attribute-publisher", federation);
         joined[0] = true;
         flavor.joinFederationExecution("multi-attribute-flavor", federation);
         joined[1] = true;
         organic.joinFederationExecution("multi-attribute-organic", federation);
         joined[2] = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         flavorClass = flavor.getObjectClassHandle(objectClassName);
         organicClass = organic.getObjectClassHandle(objectClassName);
         firstAttribute = publisher.getAttributeHandle(publisherClass, firstAttributeName);
         secondAttribute = publisher.getAttributeHandle(publisherClass, secondAttributeName);
         AttributeHandle flavorFirst = flavor.getAttributeHandle(flavorClass, firstAttributeName);
         AttributeHandle organicSecond = organic.getAttributeHandle(organicClass, secondAttributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle flavorFirstDimension = flavor.getDimensionHandle(firstDimensionName);
         DimensionHandle flavorSecondDimension = flavor.getDimensionHandle(secondDimensionName);
         DimensionHandle organicFirstDimension = organic.getDimensionHandle(firstDimensionName);
         DimensionHandle organicSecondDimension = organic.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherClass.equals(flavorClass)
               && publisherClass.equals(organicClass)
               && !firstAttribute.equals(secondAttribute)
               && firstAttribute.equals(flavorFirst)
               && secondAttribute.equals(organicSecond)
               && publisherFirst.equals(flavorFirstDimension)
               && publisherFirst.equals(organicFirstDimension)
               && publisherSecond.equals(flavorSecondDimension)
               && publisherSecond.equals(organicSecondDimension),
            "adapter multi-attribute FOM resolved inconsistent class, attribute, or dimensions");
         checkDimensions(publisher, publisherClass, publisherFirst, publisherSecond);
         checkDimensions(flavor, flavorClass, flavorFirstDimension, flavorSecondDimension);
         checkDimensions(organic, organicClass, organicFirstDimension, organicSecondDimension);

         AttributeHandleSet firstSet = JavaTckSupport.attributeSet(publisher, firstAttribute);
         AttributeHandleSet secondSet = JavaTckSupport.attributeSet(publisher, secondAttribute);
         AttributeHandleSet flavorSet = JavaTckSupport.attributeSet(flavor, flavorFirst);
         AttributeHandleSet organicSet = JavaTckSupport.attributeSet(organic, organicSecond);
         publishedAttributes = JavaTckSupport.attributeSet(
            publisher, firstAttribute, secondAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes);
         published = true;

         sourceFlavor = createRegion(publisher, publisherFirst, publisherSecond, 0, 2, 0, 2);
         sourceOrganic = createRegion(publisher, publisherFirst, publisherSecond, 5, 7, 5, 7);
         flavorRegion = createRegion(flavor, flavorFirstDimension, flavorSecondDimension,
            0, 2, 0, 2);
         organicRegion = createRegion(organic, organicFirstDimension, organicSecondDimension,
            5, 7, 5, 7);
         sourcePairs = pairList(publisher, firstSet, sourceFlavor, secondSet, sourceOrganic);
         flavorPairs = pairList(flavor, flavorSet, flavorRegion);
         organicPairs = pairList(organic, organicSet, organicRegion);

         flavor.setConveyRegionDesignatorSetsSwitch(true);
         organic.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(flavor.getConveyRegionDesignatorSetsSwitch()
               && organic.getConveyRegionDesignatorSetsSwitch(),
            "multi-attribute regional subscribers did not enable region designators");
         flavor.subscribeObjectClassAttributesWithRegions(flavorClass, flavorPairs);
         flavorSubscribed = true;
         organic.subscribeObjectClassAttributesWithRegions(organicClass, organicPairs);
         organicSubscribed = true;

         object = publisher.registerObjectInstanceWithRegions(publisherClass, sourcePairs);
         JavaTckSupport.check(object != null,
            "multi-attribute regional registration returned a null object handle");
         ObjectInstanceHandle registeredObject = object;
         awaitBoth(flavor, organic, callbackModel,
            () -> !flavorCallbacks.discoveries.isEmpty()
               && !organicCallbacks.discoveries.isEmpty(),
            "multi-attribute regional object discovery");
         JavaTckSupport.check(flavorClass.equals(flavorCallbacks.discoveries.get(0).objectClass)
               && organicClass.equals(organicCallbacks.discoveries.get(0).objectClass)
               && registeredObject.equals(flavorCallbacks.discoveries.get(0).object)
               && registeredObject.equals(organicCallbacks.discoveries.get(0).object),
            "multi-attribute regional discovery returned the wrong object class or handle");

         updateBoth(publisher, object, firstAttribute, secondAttribute,
            new byte[] {0x46, 0x31}, new byte[] {0x4f, 0x31}, new byte[] {0x49, 0x31});
         awaitBoth(flavor, organic, callbackModel,
            () -> !flavorCallbacks.reflections.isEmpty() && !organicCallbacks.reflections.isEmpty(),
            "multi-attribute regional initial reflections");
         assertReflection(flavorCallbacks.reflections.get(0), object, flavorFirst,
            publisherFederate, new byte[] {0x46, 0x31}, new byte[] {0x49, 0x31},
            sourceFlavor, flavor, "initial Flavor projection");
         assertReflection(organicCallbacks.reflections.get(0), object, organicSecond,
            publisherFederate, new byte[] {0x4f, 0x31}, new byte[] {0x49, 0x31},
            sourceOrganic, organic, "initial Organic projection");

         int flavorBeforeXDisjoint = flavorCallbacks.reflections.size();
         int organicBeforeXDisjoint = organicCallbacks.reflections.size();
         publisher.setRangeBounds(sourceFlavor, publisherFirst, new RangeBounds(8, 9));
         commitRegion(publisher, sourceFlavor);
         updateBoth(publisher, object, firstAttribute, secondAttribute,
            new byte[] {0x46, 0x58}, new byte[] {0x4f, 0x58}, new byte[] {0x58, 0x31});
         await(organic, callbackModel,
            () -> organicCallbacks.reflections.size() > organicBeforeXDisjoint,
            "surviving Organic reflection after the Flavor source range moves");
         settle(flavor, callbackModel);
         JavaTckSupport.check(flavorCallbacks.reflections.size() == flavorBeforeXDisjoint,
            "X-only source mutation leaked delivery for the independently associated attribute");
         assertReflection(organicCallbacks.reflections.get(organicBeforeXDisjoint), object,
            organicSecond, publisherFederate, new byte[] {0x4f, 0x58}, new byte[] {0x58, 0x31},
            sourceOrganic, organic, "Organic projection after the Flavor range moves");

         publisher.setRangeBounds(sourceOrganic, publisherSecond, new RangeBounds(8, 9));
         commitRegion(publisher, sourceOrganic);
         int flavorBeforeFullyDisjoint = flavorCallbacks.reflections.size();
         int organicBeforeFullyDisjoint = organicCallbacks.reflections.size();
         updateBoth(publisher, object, firstAttribute, secondAttribute,
            new byte[] {0x46, 0x4e}, new byte[] {0x4f, 0x4e}, new byte[] {0x4e, 0x31});
         settleBoth(flavor, organic, callbackModel);
         JavaTckSupport.check(flavorCallbacks.reflections.size() == flavorBeforeFullyDisjoint
               && organicCallbacks.reflections.size() == organicBeforeFullyDisjoint,
            "fully disjoint source regions delivered an update");

         publisher.setRangeBounds(sourceFlavor, publisherFirst, new RangeBounds(0, 2));
         publisher.setRangeBounds(sourceOrganic, publisherSecond, new RangeBounds(5, 7));
         commitRegions(publisher, sourceFlavor, sourceOrganic);
         int flavorBeforeRestore = flavorCallbacks.reflections.size();
         int organicBeforeRestore = organicCallbacks.reflections.size();
         updateBoth(publisher, object, firstAttribute, secondAttribute,
            new byte[] {0x46, 0x52}, new byte[] {0x4f, 0x52}, new byte[] {0x52, 0x31});
         awaitBoth(flavor, organic, callbackModel,
            () -> flavorCallbacks.reflections.size() > flavorBeforeRestore
               && organicCallbacks.reflections.size() > organicBeforeRestore,
            "restored multi-attribute regional reflections");
         assertReflection(flavorCallbacks.reflections.get(flavorBeforeRestore), object,
            flavorFirst, publisherFederate, new byte[] {0x46, 0x52}, new byte[] {0x52, 0x31},
            sourceFlavor, flavor, "restored Flavor projection");
         assertReflection(organicCallbacks.reflections.get(organicBeforeRestore), object,
            organicSecond, publisherFederate, new byte[] {0x4f, 0x52}, new byte[] {0x52, 0x31},
            sourceOrganic, organic, "restored Organic projection");
      } finally {
         if (joined[1] && flavorSubscribed && flavorPairs != null && flavorClass != null) {
            try { flavor.unsubscribeObjectClassAttributesWithRegions(flavorClass, flavorPairs); }
            catch (Exception ignored) { }
         }
         if (joined[2] && organicSubscribed && organicPairs != null && organicClass != null) {
            try { organic.unsubscribeObjectClassAttributesWithRegions(organicClass, organicPairs); }
            catch (Exception ignored) { }
         }
         if (joined[0] && object != null && sourcePairs != null) {
            try { publisher.unassociateRegionsForUpdates(object, sourcePairs); }
            catch (Exception ignored) { }
         }
         if (joined[0] && published && publisherClass != null && publishedAttributes != null) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publishedAttributes); }
            catch (Exception ignored) { }
         }
         if (joined[1] && flavorRegion != null) {
            try { flavor.deleteRegion(flavorRegion); } catch (Exception ignored) { }
         }
         if (joined[2] && organicRegion != null) {
            try { organic.deleteRegion(organicRegion); } catch (Exception ignored) { }
         }
         if (joined[0] && sourceFlavor != null) {
            try { publisher.deleteRegion(sourceFlavor); } catch (Exception ignored) { }
         }
         if (joined[0] && sourceOrganic != null) {
            try { publisher.deleteRegion(sourceOrganic); } catch (Exception ignored) { }
         }
         if (joined[1]) resign(flavor, ResignAction.NO_ACTION);
         if (joined[2]) resign(organic, ResignAction.NO_ACTION);
         if (joined[0]) resign(publisher, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected[2]) try { organic.disconnect(); } catch (Exception ignored) { }
         if (connected[1]) try { flavor.disconnect(); } catch (Exception ignored) { }
         if (connected[0]) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      JavaTckSupport.check(!first.equals(second)
            && ambassador.getAvailableDimensionsForObjectClass(objectClass).contains(first)
            && ambassador.getAvailableDimensionsForObjectClass(objectClass).contains(second)
            && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         "multi-attribute FOM must expose two distinct object dimensions through range 9");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long firstLower, long firstUpper,
         long secondLower, long secondUpper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, second, new RangeBounds(secondLower, secondUpper));
      commitRegion(ambassador, region);
      return region;
   }

   private static AttributeSetRegionSetPairList pairList(RTIambassador ambassador,
         AttributeHandleSet firstAttributes, RegionHandle firstRegion,
         AttributeHandleSet secondAttributes, RegionHandle secondRegion) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(2);
      result.add(new AttributeRegionAssociation(firstAttributes, regionSet(ambassador, firstRegion)));
      result.add(new AttributeRegionAssociation(secondAttributes, regionSet(ambassador, secondRegion)));
      return result;
   }

   private static AttributeSetRegionSetPairList pairList(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regionSet(ambassador, region)));
      return result;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static void commitRegion(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      ambassador.commitRegionModifications(regionSet(ambassador, region));
   }

   private static void commitRegions(RTIambassador ambassador, RegionHandle... regions)
         throws Exception {
      RegionHandleSet set = ambassador.getRegionHandleSetFactory().create();
      for (RegionHandle region : regions) set.add(region);
      ambassador.commitRegionModifications(set);
   }

   private static void updateBoth(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle firstAttribute, AttributeHandle secondAttribute,
         byte[] firstValue, byte[] secondValue, byte[] tag) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(2);
      values.put(firstAttribute, firstValue.clone());
      values.put(secondAttribute, secondValue.clone());
      publisher.updateAttributeValues(object, values, tag.clone());
   }

   private static void assertReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, FederateHandle producer, byte[] expectedValue, byte[] expectedTag,
         RegionHandle sourceRegion, RTIambassador receiver, String description) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(expectedValue, reflection.values.get(attribute))
            && Arrays.equals(expectedTag, reflection.tag)
            && producer.equals(reflection.producer),
         description + " returned the wrong projected value or metadata");
      JavaTckSupport.check(reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty()
            && reflection.regions != null && reflection.regions.size() == 1
            && reflection.regions.contains(sourceRegion),
         description + " did not preserve its transportation or source-region metadata");
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel callbackModel, java.util.function.BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.01);
            second.evokeCallback(0.01);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
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

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else {
         long deadline = System.nanoTime() + 100_000_000L;
         while (System.nanoTime() < deadline) Thread.yield();
      }
   }

   private static void settleBoth(RTIambassador first, RTIambassador second,
         CallbackModel callbackModel) throws Exception {
      settle(first, callbackModel);
      settle(second, callbackModel);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
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
                  if ("toString".equals(name)) return "Java TCK regional multi-attribute callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
