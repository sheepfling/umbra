package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Set;
import java.util.UUID;

/** Verifies that unpublishing releases an associated region's update-use dependency. */
final class RegionalUnpublishRegionReleaseTck {
   private RegionalUnpublishRegionReleaseTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      String federation = "java-tck-regional-unpublish-region-release-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false, published = false;
      RegionHandle region = null;
      try {
         owner.connect(noopFederateAmbassador(), callbackModel);
         connected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-regional-unpublish-owner", federation);
         joined = true;

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         DimensionHandle firstDimension = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle secondDimension = owner.getDimensionHandle(secondDimensionName);
         Set<DimensionHandle> available = owner.getAvailableDimensionsForObjectClass(objectClass);
         JavaTckSupport.check(available.contains(firstDimension)
               && available.contains(secondDimension)
               && owner.getDimensionUpperBound(firstDimension) >= 5
               && owner.getDimensionUpperBound(secondDimension) >= 5,
            "adapter object class must expose the two configured DDM dimensions with adequate bounds");

         AttributeHandleSet attributes = JavaTckSupport.attributeSet(owner, attribute);
         owner.publishObjectClassAttributes(objectClass, attributes);
         published = true;
         region = createRegion(owner, firstDimension, secondDimension);
         AttributeSetRegionSetPairList association = association(owner, attributes, region);
         ObjectInstanceHandle object =
            owner.registerObjectInstanceWithRegions(objectClass, association);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional registration returned an invalid object handle");

         RegionHandle registeredRegion = region;
         JavaTckSupport.expectFailure(() -> owner.deleteRegion(registeredRegion),
            "RegionInUseForUpdateOrSubscription",
            "deleting a region used by an object update association");

         owner.unpublishObjectClassAttributes(objectClass, attributes);
         published = false;
         owner.deleteRegion(region);
         region = null;
         JavaTckSupport.expectFailure(() -> owner.getDimensionHandleSet(registeredRegion),
            "InvalidRegion", "looking up dimensions after the region has been released");

         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         joined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (published && joined) try {
            ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
            AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
            owner.unpublishObjectClassAttributes(
               objectClass, JavaTckSupport.attributeSet(owner, attribute));
         } catch (Exception ignored) { }
         if (region != null && joined) try { owner.deleteRegion(region); }
            catch (Exception ignored) { }
         if (joined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static RegionHandle createRegion(RTIambassador owner, DimensionHandle first,
         DimensionHandle second) throws Exception {
      DimensionHandleSet dimensions = owner.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = owner.createRegion(dimensions);
      owner.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(1, 5));
      owner.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(1, 5));
      RegionHandleSet regions = owner.getRegionHandleSetFactory().create();
      regions.add(region);
      owner.commitRegionModifications(regions);
      return region;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador owner,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      RegionHandleSet regions = owner.getRegionHandleSetFactory().create();
      regions.add(region);
      AttributeSetRegionSetPairList associations =
         owner.getAttributeSetRegionSetPairListFactory().create(1);
      associations.add(new AttributeRegionAssociation(attributes, regions));
      return associations;
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK regional unpublish owner";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }
}
