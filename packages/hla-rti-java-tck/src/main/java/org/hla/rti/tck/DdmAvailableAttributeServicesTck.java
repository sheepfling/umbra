package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks attribute availability on base and derived classes across ordinary and regional declarations. */
final class DdmAvailableAttributeServicesTck {
   private DdmAvailableAttributeServicesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseObjectClassName, String derivedObjectClassName,
         String baseAttributeName, String derivedAttributeName,
         String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-ddm-available-attributes-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean basePublished = false;
      boolean derivedPublished = false;
      boolean baseSubscribed = false;
      boolean derivedBaseSubscribed = false;
      boolean derivedAttributeSubscribed = false;
      boolean baseRegionalSubscribed = false;
      boolean derivedRegionalSubscribed = false;
      ObjectClassHandle baseClass = null;
      ObjectClassHandle derivedClass = null;
      AttributeHandleSet baseAttributes = null;
      AttributeHandleSet derivedAttributes = null;
      AttributeSetRegionSetPairList baseRegionPair = null;
      AttributeSetRegionSetPairList derivedRegionPair = null;
      RegionHandle region = null;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, new String[] {fom}, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-ddm-available-attributes", federation);
         joined = true;

         baseClass = ambassador.getObjectClassHandle(baseObjectClassName);
         derivedClass = ambassador.getObjectClassHandle(derivedObjectClassName);
         AttributeHandle baseAttribute = ambassador.getAttributeHandle(
            baseClass, baseAttributeName);
         AttributeHandle derivedAttribute = ambassador.getAttributeHandle(
            derivedClass, derivedAttributeName);
         JavaTckSupport.check(!baseClass.equals(derivedClass)
               && baseAttribute != null && derivedAttribute != null,
            "adapter DDM FOM lacks the configured base/derived class attributes");

         DimensionHandle first = ambassador.getDimensionHandle(firstDimensionName);
         DimensionHandle second = ambassador.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(!first.equals(second)
               && ambassador.getAvailableDimensionsForObjectClass(baseClass).contains(first)
               && ambassador.getAvailableDimensionsForObjectClass(baseClass).contains(second)
               && ambassador.getAvailableDimensionsForObjectClass(derivedClass).contains(first)
               && ambassador.getAvailableDimensionsForObjectClass(derivedClass).contains(second)
               && ambassador.getDimensionUpperBound(first) >= 3
               && ambassador.getDimensionUpperBound(second) >= 3,
            "adapter DDM FOM did not provide the configured two-dimensional base/derived space");

         baseAttributes = JavaTckSupport.attributeSet(ambassador, baseAttribute);
         derivedAttributes = JavaTckSupport.attributeSet(ambassador, derivedAttribute);
         region = createRegion(ambassador, first, second);
         baseRegionPair = association(ambassador, baseAttributes, region);
         derivedRegionPair = association(ambassador, derivedAttributes, region);
         ObjectClassHandle baseClassForChecks = baseClass;
         AttributeHandleSet derivedAttributesForChecks = derivedAttributes;
         AttributeSetRegionSetPairList derivedRegionPairForChecks = derivedRegionPair;

         // §9.1.5 requires each declaration's attribute set to be available on its class.
         // AttributeNotDefined is checked as observed API behavior, not mandated by §9.1.5.
         ambassador.publishObjectClassAttributes(baseClass, baseAttributes);
         basePublished = true;
         JavaTckSupport.expectFailure(
            () -> ambassador.publishObjectClassAttributes(
               baseClassForChecks, derivedAttributesForChecks),
            "AttributeNotDefined", "publishing a derived-only attribute on the DDM base class");
         JavaTckSupport.expectFailure(
            () -> ambassador.unpublishObjectClassAttributes(
               baseClassForChecks, derivedAttributesForChecks),
            "AttributeNotDefined", "unpublishing a derived-only attribute on the DDM base class");
         ambassador.publishObjectClassAttributes(derivedClass, derivedAttributes);
         derivedPublished = true;
         ambassador.unpublishObjectClassAttributes(derivedClass, derivedAttributes);
         derivedPublished = false;
         ambassador.unpublishObjectClassAttributes(derivedClass, baseAttributes);
         ambassador.unpublishObjectClassAttributes(baseClass, baseAttributes);
         basePublished = false;

         ambassador.subscribeObjectClassAttributes(baseClass, baseAttributes);
         baseSubscribed = true;
         JavaTckSupport.expectFailure(
            () -> ambassador.subscribeObjectClassAttributes(
               baseClassForChecks, derivedAttributesForChecks),
            "AttributeNotDefined", "subscribing to a derived-only attribute on the DDM base class");
         JavaTckSupport.expectFailure(
            () -> ambassador.unsubscribeObjectClassAttributes(
               baseClassForChecks, derivedAttributesForChecks),
            "AttributeNotDefined", "unsubscribing a derived-only attribute on the DDM base class");
         ambassador.subscribeObjectClassAttributes(derivedClass, baseAttributes);
         derivedBaseSubscribed = true;
         ambassador.subscribeObjectClassAttributes(derivedClass, derivedAttributes);
         derivedAttributeSubscribed = true;
         ambassador.unsubscribeObjectClassAttributes(derivedClass, derivedAttributes);
         derivedAttributeSubscribed = false;
         ambassador.unsubscribeObjectClassAttributes(derivedClass, baseAttributes);
         derivedBaseSubscribed = false;
         ambassador.unsubscribeObjectClassAttributes(baseClass, baseAttributes);
         baseSubscribed = false;

         JavaTckSupport.expectFailure(
            () -> ambassador.subscribeObjectClassAttributesWithRegions(
               baseClassForChecks, derivedRegionPairForChecks),
            "AttributeNotDefined",
            "subscribing with regions to a derived-only attribute on the DDM base class");
         JavaTckSupport.expectFailure(
            () -> ambassador.unsubscribeObjectClassAttributesWithRegions(
               baseClassForChecks, derivedRegionPairForChecks),
            "AttributeNotDefined",
            "unsubscribing with regions a derived-only attribute on the DDM base class");
         ambassador.subscribeObjectClassAttributesWithRegions(baseClass, baseRegionPair);
         baseRegionalSubscribed = true;
         ambassador.unsubscribeObjectClassAttributesWithRegions(baseClass, baseRegionPair);
         baseRegionalSubscribed = false;
         ambassador.subscribeObjectClassAttributesWithRegions(derivedClass, derivedRegionPair);
         derivedRegionalSubscribed = true;
         ambassador.unsubscribeObjectClassAttributesWithRegions(derivedClass, derivedRegionPair);
         derivedRegionalSubscribed = false;
      } finally {
         if (joined && derivedRegionalSubscribed && derivedClass != null && derivedRegionPair != null) {
            try { ambassador.unsubscribeObjectClassAttributesWithRegions(
               derivedClass, derivedRegionPair); } catch (Exception ignored) { }
         }
         if (joined && baseRegionalSubscribed && baseClass != null && baseRegionPair != null) {
            try { ambassador.unsubscribeObjectClassAttributesWithRegions(
               baseClass, baseRegionPair); } catch (Exception ignored) { }
         }
         if (joined && derivedAttributeSubscribed && derivedClass != null && derivedAttributes != null) {
            try { ambassador.unsubscribeObjectClassAttributes(derivedClass, derivedAttributes); }
            catch (Exception ignored) { }
         }
         if (joined && derivedBaseSubscribed && derivedClass != null && baseAttributes != null) {
            try { ambassador.unsubscribeObjectClassAttributes(derivedClass, baseAttributes); }
            catch (Exception ignored) { }
         }
         if (joined && baseSubscribed && baseClass != null && baseAttributes != null) {
            try { ambassador.unsubscribeObjectClassAttributes(baseClass, baseAttributes); }
            catch (Exception ignored) { }
         }
         if (joined && derivedPublished && derivedClass != null && derivedAttributes != null) {
            try { ambassador.unpublishObjectClassAttributes(derivedClass, derivedAttributes); }
            catch (Exception ignored) { }
         }
         if (joined && basePublished && baseClass != null && baseAttributes != null) {
            try { ambassador.unpublishObjectClassAttributes(baseClass, baseAttributes); }
            catch (Exception ignored) { }
         }
         if (joined && region != null) {
            try { ambassador.deleteRegion(region); } catch (Exception ignored) { }
         }
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle result = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(result, first, new RangeBounds(0, 2));
      ambassador.setRangeBounds(result, second, new RangeBounds(0, 2));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(result);
      ambassador.commitRegionModifications(regions);
      return result;
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
}
