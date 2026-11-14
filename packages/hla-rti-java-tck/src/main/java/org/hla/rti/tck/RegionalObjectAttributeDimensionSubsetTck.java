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
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks region-dimension compatibility for object-attribute update associations. */
final class RegionalObjectAttributeDimensionSubsetTck {
   private RegionalObjectAttributeDimensionSubsetTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, String unrelatedDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-regional-object-dimension-subset-" + UUID.randomUUID();
      boolean ownerConnected = false, memberConnected = false, federationCreated = false;
      boolean ownerJoined = false, memberJoined = false, published = false;
      boolean validAssociationActive = false;
      ObjectClassHandle objectClass = null;
      AttributeHandleSet attributes = null;
      AttributeSetRegionSetPairList validPairs = null, unrelatedPairs = null;
      RegionHandle validRegion = null, unrelatedRegion = null;
      ObjectInstanceHandle object = null;
      ObjectInstanceHandle[] unexpectedInvalidRegistration = new ObjectInstanceHandle[1];
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         owner.joinFederationExecution("java-tck-object-dimension-subset-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-object-dimension-subset-member", federation);
         memberJoined = true;

         objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         DimensionHandle first = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle second = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle unrelated = owner.getDimensionHandle(unrelatedDimensionName);
         DimensionHandleSet available = owner.getAvailableDimensionsForObjectClass(objectClass);
         JavaTckSupport.check(available.contains(first) && available.contains(second)
               && !available.contains(unrelated),
            "adapter object class must expose the two configured dimensions and exclude the unrelated one");

         attributes = JavaTckSupport.attributeSet(owner, attribute);
         owner.publishObjectClassAttributes(objectClass, attributes);
         published = true;

         unrelatedRegion = createRegion(owner, new DimensionHandle[] {unrelated},
            new long[] {0}, new long[] {1});
         unrelatedPairs = association(owner, attributes, unrelatedRegion);
         ObjectClassHandle publishedClass = objectClass;
         AttributeSetRegionSetPairList invalidRegistrationPairs = unrelatedPairs;
         JavaTckSupport.expectFailure(() -> unexpectedInvalidRegistration[0] =
               owner.registerObjectInstanceWithRegions(publishedClass, invalidRegistrationPairs),
            "InvalidRegionContext",
            "regional object registration with a dimension unavailable to its attribute");

         validRegion = createRegion(owner, new DimensionHandle[] {first, second},
            new long[] {1, 2}, new long[] {5, 7});
         validPairs = association(owner, attributes, validRegion);
         object = owner.registerObjectInstanceWithRegions(objectClass, validPairs);
         validAssociationActive = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "registration with the available object-attribute dimensions failed");

         ObjectInstanceHandle registeredObject = object;
         AttributeSetRegionSetPairList invalidAssociationPairs = unrelatedPairs;
         JavaTckSupport.expectFailure(() -> owner.associateRegionsForUpdates(
               registeredObject, invalidAssociationPairs),
            "InvalidRegionContext",
            "region association with a dimension unavailable to the instance attribute");

         owner.unassociateRegionsForUpdates(object, validPairs);
         validAssociationActive = false;
         owner.deleteRegion(validRegion);
         validRegion = null;
         owner.deleteRegion(unrelatedRegion);
         unrelatedRegion = null;
      } finally {
         if (ownerJoined && unexpectedInvalidRegistration[0] != null && unrelatedPairs != null) {
            try { owner.unassociateRegionsForUpdates(unexpectedInvalidRegistration[0], unrelatedPairs); }
            catch (Exception ignored) { }
            try { owner.deleteObjectInstance(unexpectedInvalidRegistration[0], new byte[0]); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && object != null && unrelatedPairs != null) {
            try { owner.unassociateRegionsForUpdates(object, unrelatedPairs); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && object != null && validAssociationActive && validPairs != null) {
            try { owner.unassociateRegionsForUpdates(object, validPairs); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published && objectClass != null && attributes != null) {
            try { owner.unpublishObjectClassAttributes(objectClass, attributes); }
            catch (Exception ignored) { }
         }
         deleteRegion(owner, validRegion, ownerJoined);
         deleteRegion(owner, unrelatedRegion, ownerJoined);
         if (memberJoined) resign(member);
         if (ownerJoined) resign(owner);
         if (federationCreated) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (memberConnected) {
            try { member.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador,
         DimensionHandle[] dimensions, long[] lowerBounds, long[] upperBounds) throws Exception {
      JavaTckSupport.check(dimensions.length == lowerBounds.length
            && dimensions.length == upperBounds.length,
         "region range fixture must provide one bound pair per dimension");
      DimensionHandleSet regionDimensions = ambassador.getDimensionHandleSetFactory().create();
      for (DimensionHandle dimension : dimensions) regionDimensions.add(dimension);
      RegionHandle region = ambassador.createRegion(regionDimensions);
      for (int index = 0; index < dimensions.length; ++index) {
         ambassador.setRangeBounds(region, dimensions[index],
            new RangeBounds(lowerBounds[index], upperBounds[index]));
      }
      RegionHandleSet regionSet = ambassador.getRegionHandleSetFactory().create();
      regionSet.add(region);
      ambassador.commitRegionModifications(regionSet);
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

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean joined) {
      if (joined && region != null) {
         try { ambassador.deleteRegion(region); } catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK object-attribute dimension subset callbacks";
               }
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }
}
