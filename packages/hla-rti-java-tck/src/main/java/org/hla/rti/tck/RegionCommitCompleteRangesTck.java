package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks incomplete region commit rejection and recovery through the public API. */
final class RegionCommitCompleteRangesTck {
   private RegionCommitCompleteRangesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-region-commit-complete-ranges-" + UUID.randomUUID();
      boolean ownerConnected = false, memberConnected = false, federationCreated = false;
      boolean ownerJoined = false, memberJoined = false, published = false;
      ObjectClassHandle objectClass = null;
      AttributeHandleSet attributes = null;
      RegionHandle region = null;
      ObjectInstanceHandle object = null;
      ObjectInstanceHandle[] unexpectedIncompleteRegistration = new ObjectInstanceHandle[1];
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         owner.joinFederationExecution("java-tck-region-commit-ranges-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-region-commit-ranges-member", federation);
         memberJoined = true;

         objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         InteractionClassHandle interactionClass =
            owner.getInteractionClassHandle(interactionClassName);
         ParameterHandle parameter = owner.getParameterHandle(interactionClass, parameterName);
         DimensionHandle first = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle second = owner.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(objectClass != null && objectClass.encodedLength() > 0
               && attribute != null && attribute.encodedLength() > 0
               && interactionClass != null && interactionClass.encodedLength() > 0
               && parameter != null && parameter.encodedLength() > 0
               && first != null && first.encodedLength() > 0
               && second != null && second.encodedLength() > 0,
            "DDM handle lookup returned an invalid handle");
         DimensionHandleSet objectDimensions =
            owner.getAvailableDimensionsForObjectClass(objectClass);
         DimensionHandleSet interactionDimensions =
            owner.getAvailableDimensionsForInteractionClass(interactionClass);
         JavaTckSupport.check(!first.equals(second),
            "DDM adapter supplied duplicate dimension names");
         JavaTckSupport.check(objectDimensions.contains(first) && objectDimensions.contains(second)
               && interactionDimensions.contains(first) && interactionDimensions.contains(second),
            "adapter object and interaction classes must expose both configured dimensions");
         JavaTckSupport.check(firstDimensionName.equals(owner.getDimensionName(first))
               && secondDimensionName.equals(owner.getDimensionName(second)),
            "dimension-name lookup returned a different name");
         JavaTckSupport.check(owner.getDimensionUpperBound(first) >= 4
               && owner.getDimensionUpperBound(second) >= 4,
            "adapter-supplied dimensions have insufficient range");

         attributes = JavaTckSupport.attributeSet(owner, attribute);
         owner.publishObjectClassAttributes(objectClass, attributes);
         published = true;

         DimensionHandleSet dimensions = owner.getDimensionHandleSetFactory().create();
         dimensions.add(first);
         dimensions.add(second);
         region = owner.createRegion(dimensions);
         JavaTckSupport.check(region != null && region.encodedLength() > 0,
            "two-dimensional region template creation failed");
         JavaTckSupport.check(owner.getDimensionHandleSet(region).equals(dimensions),
            "Get Dimension Handle Set did not accept the region template");

         owner.setRangeBounds(region, first, new RangeBounds(1, 5));
         RegionHandleSet regions = regionSet(owner, region);
         JavaTckSupport.expectFailure(() -> owner.commitRegionModifications(regions),
            "InvalidRegion", "committing a region template with one missing range");

         RangeBounds retainedFirstBounds = owner.getRangeBounds(region, first);
         JavaTckSupport.check(retainedFirstBounds.lower == 1
               && retainedFirstBounds.upper == 5,
            "failed incomplete commit discarded the pending first-dimension bounds");

         AttributeSetRegionSetPairList attributeRegions =
            association(owner, attributes, regions);
         ObjectClassHandle publishedClass = objectClass;
         JavaTckSupport.expectFailure(() -> unexpectedIncompleteRegistration[0] =
               owner.registerObjectInstanceWithRegions(publishedClass, attributeRegions),
            "InvalidRegion", "registering an object with an incomplete region template");

         owner.setRangeBounds(region, second, new RangeBounds(6, 9));
         owner.commitRegionModifications(regions);
         JavaTckSupport.check(owner.getDimensionHandleSet(region).equals(dimensions),
            "committed region specification changed its dimension set");
         RangeBounds completedFirstBounds = owner.getRangeBounds(region, first);
         RangeBounds completedSecondBounds = owner.getRangeBounds(region, second);
         JavaTckSupport.check(completedFirstBounds.lower == 1
               && completedFirstBounds.upper == 5
               && completedSecondBounds.lower == 6
               && completedSecondBounds.upper == 9,
            "committed region specification did not retain both completed ranges");

         object = owner.registerObjectInstanceWithRegions(objectClass, attributeRegions);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "completed region template was not usable for registration");
         owner.deleteObjectInstance(object, new byte[0]);
         object = null;

         owner.unpublishObjectClassAttributes(objectClass, attributes);
         published = false;
         owner.deleteRegion(region);
         region = null;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         owner.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (ownerJoined && unexpectedIncompleteRegistration[0] != null) try {
            owner.deleteObjectInstance(unexpectedIncompleteRegistration[0], new byte[0]);
         } catch (Exception ignored) { }
         if (ownerJoined && object != null) try {
            owner.deleteObjectInstance(object, new byte[0]);
         } catch (Exception ignored) { }
         if (ownerJoined && published && objectClass != null && attributes != null) try {
            owner.unpublishObjectClassAttributes(objectClass, attributes);
         } catch (Exception ignored) { }
         deleteRegion(owner, region, ownerJoined);
         if (memberJoined) resign(member);
         if (ownerJoined) resign(owner);
         if (federationCreated) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandleSet regions) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions));
      return result;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean joined) {
      if (joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK region commit complete ranges callbacks";
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
