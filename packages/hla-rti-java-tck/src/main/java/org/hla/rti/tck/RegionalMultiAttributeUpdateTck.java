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
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

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
      boolean flavorCrossProductSubscribed = false;
      boolean organicCrossProductSubscribed = false;
      RegionHandle sourceFlavor = null;
      RegionHandle sourceOrganic = null;
      RegionHandle flavorRegion = null;
      RegionHandle organicRegion = null;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle flavorClass = null;
      ObjectClassHandle organicClass = null;
      AttributeHandle firstAttribute = null;
      AttributeHandle secondAttribute = null;
      AttributeHandleSet flavorCrossProductAttributes = null;
      AttributeHandleSet organicCrossProductAttributes = null;
      AttributeHandleSet publishedAttributes = null;
      AttributeSetRegionSetPairList sourcePairs = null;
      AttributeSetRegionSetPairList sourceCrossProductPair = null;
      AttributeSetRegionSetPairList flavorPairs = null;
      AttributeSetRegionSetPairList organicPairs = null;
      AttributeSetRegionSetPairList flavorCrossProductPair = null;
      AttributeSetRegionSetPairList organicCrossProductPair = null;
      ObjectInstanceHandle object = null;
      ObjectInstanceHandle crossProductObject = null;
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
         AttributeHandle flavorSecond = flavor.getAttributeHandle(flavorClass, secondAttributeName);
         AttributeHandle organicFirst = organic.getAttributeHandle(organicClass, firstAttributeName);
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
               && secondAttribute.equals(flavorSecond)
               && firstAttribute.equals(organicFirst)
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
         flavorCrossProductAttributes = JavaTckSupport.attributeSet(
            flavor, flavorFirst, flavorSecond);
         organicCrossProductAttributes = JavaTckSupport.attributeSet(
            organic, organicFirst, organicSecond);
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
         checkCallbackFailures(publisherCallbacks, flavorCallbacks, organicCallbacks);
         JavaTckSupport.check(flavorClass.equals(flavorCallbacks.discoveries.get(0).objectClass)
               && organicClass.equals(organicCallbacks.discoveries.get(0).objectClass)
               && registeredObject.equals(flavorCallbacks.discoveries.get(0).object)
               && registeredObject.equals(organicCallbacks.discoveries.get(0).object),
            "multi-attribute regional discovery returned the wrong object class or handle");

         installSingleSourceRegionInspection(flavorCallbacks, flavor,
            flavorFirstDimension, flavorSecondDimension, 0, 2, "Flavor");
         installSingleSourceRegionInspection(organicCallbacks, organic,
            organicFirstDimension, organicSecondDimension, 5, 7, "Organic");

         updateBoth(publisher, object, firstAttribute, secondAttribute,
            new byte[] {0x46, 0x31}, new byte[] {0x4f, 0x31}, new byte[] {0x49, 0x31});
         awaitBoth(flavor, organic, callbackModel,
            () -> !flavorCallbacks.reflections.isEmpty() && !organicCallbacks.reflections.isEmpty(),
            "multi-attribute regional initial reflections");
         settleBoth(flavor, organic, callbackModel);
         checkCallbackFailures(publisherCallbacks, flavorCallbacks, organicCallbacks);
         assertReflection(flavorCallbacks.reflections.get(0), object, flavorFirst,
            publisherFederate, new byte[] {0x46, 0x31}, new byte[] {0x49, 0x31},
            flavor, flavorCallbacks, "initial Flavor projection");
         assertReflection(organicCallbacks.reflections.get(0), object, organicSecond,
            publisherFederate, new byte[] {0x4f, 0x31}, new byte[] {0x49, 0x31},
            organic, organicCallbacks, "initial Organic projection");

         int flavorBeforeXDisjoint = flavorCallbacks.reflections.size();
         int organicBeforeXDisjoint = organicCallbacks.reflections.size();
         publisher.setRangeBounds(sourceFlavor, publisherFirst, new RangeBounds(8, 9));
         commitRegion(publisher, sourceFlavor);
         updateBoth(publisher, object, firstAttribute, secondAttribute,
            new byte[] {0x46, 0x58}, new byte[] {0x4f, 0x58}, new byte[] {0x58, 0x31});
         await(organic, callbackModel,
            () -> organicCallbacks.reflections.size() > organicBeforeXDisjoint,
            "surviving Organic reflection after the Flavor source range moves");
         settleBoth(flavor, organic, callbackModel);
         checkCallbackFailures(publisherCallbacks, flavorCallbacks, organicCallbacks);
         JavaTckSupport.check(flavorCallbacks.reflections.size() == flavorBeforeXDisjoint,
            "X-only source mutation leaked delivery for the independently associated attribute");
         assertReflection(organicCallbacks.reflections.get(organicBeforeXDisjoint), object,
            organicSecond, publisherFederate, new byte[] {0x4f, 0x58}, new byte[] {0x58, 0x31},
            organic, organicCallbacks, "Organic projection after the Flavor range moves");

         publisher.setRangeBounds(sourceOrganic, publisherSecond, new RangeBounds(8, 9));
         commitRegion(publisher, sourceOrganic);
         int flavorBeforeFullyDisjoint = flavorCallbacks.reflections.size();
         int organicBeforeFullyDisjoint = organicCallbacks.reflections.size();
         updateBoth(publisher, object, firstAttribute, secondAttribute,
            new byte[] {0x46, 0x4e}, new byte[] {0x4f, 0x4e}, new byte[] {0x4e, 0x31});
         settleBoth(flavor, organic, callbackModel);
         checkCallbackFailures(publisherCallbacks, flavorCallbacks, organicCallbacks);
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
         settleBoth(flavor, organic, callbackModel);
         checkCallbackFailures(publisherCallbacks, flavorCallbacks, organicCallbacks);
         assertReflection(flavorCallbacks.reflections.get(flavorBeforeRestore), object,
            flavorFirst, publisherFederate, new byte[] {0x46, 0x52}, new byte[] {0x52, 0x31},
            flavor, flavorCallbacks, "restored Flavor projection");
         assertReflection(organicCallbacks.reflections.get(organicBeforeRestore), object,
            organicSecond, publisherFederate, new byte[] {0x4f, 0x52}, new byte[] {0x52, 0x31},
            organic, organicCallbacks, "restored Organic projection");

         flavorCrossProductPair = pairList(flavor, flavorCrossProductAttributes, flavorRegion);
         organicCrossProductPair = pairList(organic, organicCrossProductAttributes, organicRegion);
         flavor.subscribeObjectClassAttributesWithRegions(flavorClass, flavorCrossProductPair);
         flavorCrossProductSubscribed = true;
         organic.subscribeObjectClassAttributesWithRegions(organicClass, organicCrossProductPair);
         organicCrossProductSubscribed = true;
         sourceCrossProductPair = pairList(publisher, publishedAttributes,
            sourceFlavor, sourceOrganic);
         crossProductObject = publisher.registerObjectInstanceWithRegions(
            publisherClass, sourceCrossProductPair);
         JavaTckSupport.check(crossProductObject != null,
            "multi-attribute attribute-region cross-product registration returned no handle");
         ObjectInstanceHandle registeredCrossProductObject = crossProductObject;
         awaitBoth(flavor, organic, callbackModel,
            () -> flavorCallbacks.discoveries.stream().anyMatch(
                  discovery -> registeredCrossProductObject.equals(discovery.object))
               && organicCallbacks.discoveries.stream().anyMatch(
                  discovery -> registeredCrossProductObject.equals(discovery.object)),
            "multi-attribute attribute-region cross-product discovery");
         checkCallbackFailures(publisherCallbacks, flavorCallbacks, organicCallbacks);

         AtomicInteger flavorCrossProductRegionCallbacks = new AtomicInteger();
         AtomicInteger organicCrossProductRegionCallbacks = new AtomicInteger();
         installCrossProductRegionInspection(flavorCallbacks, flavor,
            flavorFirstDimension, flavorSecondDimension, registeredCrossProductObject,
            flavorCrossProductRegionCallbacks, "Flavor");
         installCrossProductRegionInspection(organicCallbacks, organic,
            organicFirstDimension, organicSecondDimension, registeredCrossProductObject,
            organicCrossProductRegionCallbacks, "Organic");

         byte[] crossProductFirstValue = {0x46, 0x58};
         byte[] crossProductSecondValue = {0x4f, 0x58};
         byte[] crossProductTag = {0x58, 0x50};
         updateBoth(publisher, crossProductObject, firstAttribute, secondAttribute,
            crossProductFirstValue, crossProductSecondValue, crossProductTag);
         awaitBoth(flavor, organic, callbackModel,
            () -> hasBothAttributes(flavorCallbacks.reflections, registeredCrossProductObject,
                  flavorFirst, flavorSecond)
               && hasBothAttributes(organicCallbacks.reflections, registeredCrossProductObject,
                  organicFirst, organicSecond),
            "multi-attribute attribute-region cross-product reflections");
         checkCallbackFailures(publisherCallbacks, flavorCallbacks, organicCallbacks);
         settleBoth(flavor, organic, callbackModel);
         checkCallbackFailures(publisherCallbacks, flavorCallbacks, organicCallbacks);
         assertCrossProductProjection(flavorCallbacks, registeredCrossProductObject,
            flavorFirst, flavorSecond, publisherFederate, crossProductFirstValue,
            crossProductSecondValue, crossProductTag, flavor, flavorCrossProductRegionCallbacks,
            "Flavor cross-product projection");
         assertCrossProductProjection(organicCallbacks, registeredCrossProductObject,
            organicFirst, organicSecond, publisherFederate, crossProductFirstValue,
            crossProductSecondValue, crossProductTag, organic, organicCrossProductRegionCallbacks,
            "Organic cross-product projection");
      } finally {
         if (joined[1] && flavorCrossProductSubscribed && flavorCrossProductPair != null
               && flavorClass != null) {
            try { flavor.unsubscribeObjectClassAttributesWithRegions(
               flavorClass, flavorCrossProductPair); }
            catch (Exception ignored) { }
         }
         if (joined[2] && organicCrossProductSubscribed && organicCrossProductPair != null
               && organicClass != null) {
            try { organic.unsubscribeObjectClassAttributesWithRegions(
               organicClass, organicCrossProductPair); }
            catch (Exception ignored) { }
         }
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
         if (joined[0] && crossProductObject != null && sourceCrossProductPair != null) {
            try { publisher.unassociateRegionsForUpdates(
               crossProductObject, sourceCrossProductPair); }
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

   private static AttributeSetRegionSetPairList pairList(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle firstRegion,
         RegionHandle secondRegion) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(
         attributes, regionSet(ambassador, firstRegion, secondRegion)));
      return result;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador,
         RegionHandle firstRegion, RegionHandle secondRegion) throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(firstRegion);
      result.add(secondRegion);
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

   private static void installSingleSourceRegionInspection(Recorder recorder,
         RTIambassador receiver, DimensionHandle firstDimension, DimensionHandle secondDimension,
         long lower, long upper, String description) {
      recorder.reflectionInspector = reflection -> {
         JavaTckSupport.check(reflection.regions != null && reflection.regions.size() == 1,
            description + " reflection omitted its callback-scoped source region copy");
         RegionHandle callbackRegion = reflection.regions.iterator().next();
         DimensionHandleSet dimensions = receiver.getDimensionHandleSet(callbackRegion);
         RangeBounds firstBounds = receiver.getRangeBounds(callbackRegion, firstDimension);
         RangeBounds secondBounds = receiver.getRangeBounds(callbackRegion, secondDimension);
         JavaTckSupport.check(dimensions.size() == 2
               && dimensions.contains(firstDimension) && dimensions.contains(secondDimension)
               && firstBounds.lower == lower && firstBounds.upper == upper
               && secondBounds.lower == lower && secondBounds.upper == upper,
            description + " callback region copy changed its dimensions or range bounds");
         recorder.inspectedRegionCallbacks.incrementAndGet();
      };
   }

   private static void installCrossProductRegionInspection(Recorder recorder,
         RTIambassador receiver, DimensionHandle firstDimension, DimensionHandle secondDimension,
         ObjectInstanceHandle object, AtomicInteger inspectedCallbacks, String description) {
      recorder.reflectionInspector = reflection -> {
         if (!object.equals(reflection.object)) return;
         JavaTckSupport.check(reflection.regions != null && !reflection.regions.isEmpty(),
            description + " cross-product reflection omitted its callback source region set");
         boolean sawMatchingSourceRegion = false;
         for (RegionHandle callbackRegion : reflection.regions) {
            DimensionHandleSet dimensions = receiver.getDimensionHandleSet(callbackRegion);
            RangeBounds firstBounds = receiver.getRangeBounds(callbackRegion, firstDimension);
            RangeBounds secondBounds = receiver.getRangeBounds(callbackRegion, secondDimension);
            boolean isFlavorRegion = firstBounds.lower == 0 && firstBounds.upper == 2
               && secondBounds.lower == 0 && secondBounds.upper == 2;
            boolean isOrganicRegion = firstBounds.lower == 5 && firstBounds.upper == 7
               && secondBounds.lower == 5 && secondBounds.upper == 7;
            JavaTckSupport.check(dimensions.size() == 2
                  && dimensions.contains(firstDimension) && dimensions.contains(secondDimension)
                  && (isFlavorRegion || isOrganicRegion),
               description + " cross-product callback copied an unknown source region");
            sawMatchingSourceRegion = sawMatchingSourceRegion
               || ("Flavor".equals(description) ? isFlavorRegion : isOrganicRegion);
         }
         JavaTckSupport.check(sawMatchingSourceRegion,
            description + " cross-product callback omitted its matching source region");
         inspectedCallbacks.incrementAndGet();
         recorder.inspectedRegionCallbacks.incrementAndGet();
      };
   }

   private static boolean hasBothAttributes(List<Reflection> reflections,
         ObjectInstanceHandle object, AttributeHandle firstAttribute,
         AttributeHandle secondAttribute) {
      boolean sawFirst = false;
      boolean sawSecond = false;
      for (Reflection reflection : reflections) {
         if (!object.equals(reflection.object)) continue;
         sawFirst |= reflection.values.containsKey(firstAttribute);
         sawSecond |= reflection.values.containsKey(secondAttribute);
      }
      return sawFirst && sawSecond;
   }

   private static void assertCrossProductProjection(Recorder recorder,
         ObjectInstanceHandle object, AttributeHandle firstAttribute,
         AttributeHandle secondAttribute, FederateHandle producer, byte[] firstValue,
         byte[] secondValue, byte[] tag, RTIambassador receiver,
         AtomicInteger inspectedCallbacks, String description) throws Exception {
      Set<AttributeHandle> observedAttributes = new HashSet<>();
      for (Reflection reflection : recorder.reflections) {
         if (!object.equals(reflection.object)) continue;
         JavaTckSupport.check(Arrays.equals(tag, reflection.tag)
               && producer.equals(reflection.producer)
               && reflection.transportation != null
               && !receiver.getTransportationTypeName(reflection.transportation).isEmpty()
               && reflection.regions != null && !reflection.regions.isEmpty(),
            description + " changed callback metadata or omitted its source region designator");
         for (Map.Entry<AttributeHandle, byte[]> value : reflection.values.entrySet()) {
            AttributeHandle attribute = value.getKey();
            JavaTckSupport.check(attribute.equals(firstAttribute)
                  || attribute.equals(secondAttribute),
               description + " included an attribute outside the requested set");
            JavaTckSupport.check(observedAttributes.add(attribute),
               description + " delivered a cross-product attribute more than once");
            byte[] expectedValue = attribute.equals(firstAttribute) ? firstValue : secondValue;
            JavaTckSupport.check(Arrays.equals(expectedValue, value.getValue()),
               description + " returned the wrong value for a cross-product attribute");
         }
      }
      JavaTckSupport.check(observedAttributes.size() == 2
            && observedAttributes.contains(firstAttribute)
            && observedAttributes.contains(secondAttribute)
            && inspectedCallbacks.get() > 0,
         description + " did not observe both attributes and their source region copies");
      JavaTckSupport.check(recorder.inspectedRegionCallbacks.get() == recorder.reflections.size(),
         description + " had a reflection callback without callback-scoped region inspection");
   }

   private static void checkCallbackFailures(Recorder... recorders) {
      for (Recorder recorder : recorders) {
         Throwable failure = recorder.callbackFailure.get();
         if (failure != null) {
            throw new AssertionError("Regional multi-attribute callback inspection failed", failure);
         }
      }
   }

   private static void assertReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, FederateHandle producer, byte[] expectedValue, byte[] expectedTag,
         RTIambassador receiver, Recorder recorder, String description) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(expectedValue, reflection.values.get(attribute))
            && Arrays.equals(expectedTag, reflection.tag)
            && producer.equals(reflection.producer),
         description + " returned the wrong projected value or metadata");
      JavaTckSupport.check(reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty()
            && reflection.regions != null && reflection.regions.size() == 1
            && recorder.inspectedRegionCallbacks.get() == recorder.reflections.size(),
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
      private final AtomicReference<Throwable> callbackFailure = new AtomicReference<>();
      private final AtomicInteger inspectedRegionCallbacks = new AtomicInteger();
      private volatile ReflectionInspector reflectionInspector;
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) discoveries.add(new Discovery(arguments));
               else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  Reflection reflection = new Reflection(arguments);
                  ReflectionInspector inspector = reflectionInspector;
                  if (inspector != null) {
                     try { inspector.inspect(reflection); }
                     catch (Throwable failure) { callbackFailure.compareAndSet(null, failure); }
                  }
                  reflections.add(reflection);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional multi-attribute callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }

   @FunctionalInterface
   private interface ReflectionInspector {
      void inspect(Reflection reflection) throws Exception;
   }
}
