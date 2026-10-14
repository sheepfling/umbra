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
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Set;
import java.util.UUID;

/** Rejects incompatible, incomplete, foreign, and in-use regional DDM inputs. */
final class RegionalBoundariesTck {
   private RegionalBoundariesTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String interactionClassName, String parameterName, String firstDimensionName,
         String secondDimensionName, String unrelatedDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador foreign = factory.getRtiAmbassador();
      String federation = "java-tck-regional-boundaries-" + UUID.randomUUID();
      boolean ownerConnected = false, foreignConnected = false, created = false;
      boolean ownerJoined = false, foreignJoined = false;
      boolean objectPublished = false, interactionPublished = false;
      boolean regionalSubscribed = false;
      RegionHandle uncommittedRegion = null, wrongContextRegion = null, foreignRegion = null;
      RegionHandle subscriptionRegion = null, objectRegion = null;
      ObjectClassHandle ownerObjectClass = null;
      AttributeHandleSet ownerAttributes = null;
      InteractionClassHandle ownerInteraction = null;
      AttributeSetRegionSetPairList subscriptionAssociation = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         foreign.connect(callbacks(), callbackModel);
         foreignConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-regional-boundaries-owner", federation);
         ownerJoined = true;
         foreign.joinFederationExecution("java-tck-regional-boundaries-foreign", federation);
         foreignJoined = true;

         ownerObjectClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle foreignObjectClass = foreign.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerObjectClass, attributeName);
         AttributeHandle foreignAttribute = foreign.getAttributeHandle(foreignObjectClass, attributeName);
         ownerInteraction = owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle foreignInteraction =
            foreign.getInteractionClassHandle(interactionClassName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle foreignParameter =
            foreign.getParameterHandle(foreignInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle ownerUnrelated = owner.getDimensionHandle(unrelatedDimensionName);
         DimensionHandle foreignFirst = foreign.getDimensionHandle(firstDimensionName);
         DimensionHandle foreignSecond = foreign.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerObjectClass.equals(foreignObjectClass)
               && ownerAttribute.equals(foreignAttribute)
               && ownerInteraction.equals(foreignInteraction)
               && ownerParameter.equals(foreignParameter)
               && ownerFirst.equals(foreignFirst) && ownerSecond.equals(foreignSecond),
            "adapter DDM FOM handles changed across regional-boundary federates");
         Set<DimensionHandle> objectDimensions = owner.getAvailableDimensionsForObjectClass(
            ownerObjectClass);
         Set<DimensionHandle> interactionDimensions =
            owner.getAvailableDimensionsForInteractionClass(ownerInteraction);
         JavaTckSupport.check(objectDimensions.contains(ownerFirst)
               && objectDimensions.contains(ownerSecond)
               && interactionDimensions.contains(ownerFirst)
               && interactionDimensions.contains(ownerSecond)
               && !objectDimensions.contains(ownerUnrelated)
               && !interactionDimensions.contains(ownerUnrelated)
               && owner.getDimensionUpperBound(ownerFirst) >= 6
               && owner.getDimensionUpperBound(ownerSecond) >= 6
               && owner.getDimensionUpperBound(ownerUnrelated) >= 5,
            "adapter DDM FOM must separate two class dimensions from the unrelated dimension");

         DimensionHandleSet noDimensions = owner.getDimensionHandleSetFactory().create();
         RegionHandle zeroDimensionalRegion = owner.createRegion(noDimensions);
         JavaTckSupport.check(owner.getDimensionHandleSet(zeroDimensionalRegion).isEmpty(),
            "zero-dimensional region unexpectedly contained dimensions");
         owner.commitRegionModifications(regionSet(owner, zeroDimensionalRegion));
         owner.deleteRegion(zeroDimensionalRegion);

         uncommittedRegion = owner.createRegion(dimensionSet(owner, ownerFirst, ownerSecond));
         RegionHandle incompleteRegion = uncommittedRegion;
         JavaTckSupport.expectFailure(() -> owner.getRangeBounds(incompleteRegion, ownerFirst),
            "InvalidRegion", "reading a range from an uncommitted region");
         JavaTckSupport.expectFailure(() -> owner.commitRegionModifications(
               regionSet(owner, incompleteRegion)),
            "InvalidRegion", "committing a region with missing range bounds");
         owner.setRangeBounds(uncommittedRegion, ownerFirst,
            new hla.rti1516_2025.RangeBounds(1, 5));
         owner.setRangeBounds(uncommittedRegion, ownerSecond,
            new hla.rti1516_2025.RangeBounds(1, 5));
         owner.commitRegionModifications(regionSet(owner, uncommittedRegion));

         wrongContextRegion = owner.createRegion(dimensionSet(owner, ownerUnrelated));
         owner.setRangeBounds(wrongContextRegion, ownerUnrelated,
            new hla.rti1516_2025.RangeBounds(1, 5));
         owner.commitRegionModifications(regionSet(owner, wrongContextRegion));
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         owner.publishObjectClassAttributes(ownerObjectClass, ownerAttributes);
         objectPublished = true;
         AttributeSetRegionSetPairList wrongObjectAssociation = association(
            owner, ownerAttributes, wrongContextRegion);
         ObjectClassHandle publishedObjectClass = ownerObjectClass;
         JavaTckSupport.expectFailure(() -> owner.registerObjectInstanceWithRegions(
               publishedObjectClass, wrongObjectAssociation),
            "InvalidRegionContext", "registering an object with an incompatible region context");

         owner.publishInteractionClass(ownerInteraction);
         interactionPublished = true;
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, new byte[] {0x42});
         InteractionClassHandle publishedInteraction = ownerInteraction;
         RegionHandle incompatibleContextRegion = wrongContextRegion;
         JavaTckSupport.expectFailure(() -> owner.sendInteractionWithRegions(publishedInteraction,
               parameters, regionSet(owner, incompatibleContextRegion), new byte[] {0x42, 0x44}),
            "InvalidRegionContext", "sending an interaction with an incompatible region context");

         foreignRegion = createRegion(foreign, foreignFirst, foreignSecond, 1, 5);
         RegionHandle anotherFederateRegion = foreignRegion;
         JavaTckSupport.expectFailure(() -> owner.deleteRegion(anotherFederateRegion),
            "RegionNotCreatedByThisFederate", "deleting a region created by another federate");
         JavaTckSupport.expectFailure(() -> owner.commitRegionModifications(
               regionSet(owner, anotherFederateRegion)),
            "RegionNotCreatedByThisFederate", "committing a region created by another federate");

         subscriptionRegion = createRegion(owner, ownerFirst, ownerSecond, 2, 6);
         RegionHandleSet subscriptionRegions = regionSet(owner, subscriptionRegion);
         owner.subscribeInteractionClassPassivelyWithRegions(ownerInteraction, subscriptionRegions);
         regionalSubscribed = true;
         RegionHandle passiveSubscriptionRegion = subscriptionRegion;
         JavaTckSupport.expectFailure(() -> owner.deleteRegion(passiveSubscriptionRegion),
            "RegionInUseForUpdateOrSubscription",
            "deleting a region used by a passive interaction subscription");
         owner.unsubscribeInteractionClassWithRegions(ownerInteraction, subscriptionRegions);
         regionalSubscribed = false;
         owner.deleteRegion(subscriptionRegion);
         subscriptionRegion = null;

         objectRegion = createRegion(owner, ownerFirst, ownerSecond, 2, 6);
         AttributeSetRegionSetPairList objectAssociation =
            association(owner, ownerAttributes, objectRegion);
         object = owner.registerObjectInstanceWithRegions(ownerObjectClass, objectAssociation);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional boundary object registration returned an invalid handle");
         RegionHandle objectUpdateRegion = objectRegion;
         JavaTckSupport.expectFailure(() -> owner.deleteRegion(objectUpdateRegion),
            "RegionInUseForUpdateOrSubscription",
            "deleting a region used by an object update association");
         owner.deleteObjectInstance(object, new byte[0]);
         object = null;
         owner.deleteRegion(objectRegion);
         objectRegion = null;

         owner.unpublishObjectClassAttributes(ownerObjectClass, ownerAttributes);
         objectPublished = false;
         owner.unpublishInteractionClass(ownerInteraction);
         interactionPublished = false;
         owner.deleteRegion(wrongContextRegion);
         wrongContextRegion = null;
         owner.deleteRegion(uncommittedRegion);
         uncommittedRegion = null;
         foreign.deleteRegion(foreignRegion);
         foreignRegion = null;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         foreign.resignFederationExecution(ResignAction.NO_ACTION);
         foreignJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (regionalSubscribed && ownerJoined) try {
            owner.unsubscribeInteractionClassWithRegions(ownerInteraction,
               regionSet(owner, subscriptionRegion));
         } catch (Exception ignored) { }
         if (object != null && ownerJoined) try { owner.deleteObjectInstance(object, new byte[0]); }
            catch (Exception ignored) { }
         if (objectPublished && ownerJoined && ownerObjectClass != null && ownerAttributes != null)
            try { owner.unpublishObjectClassAttributes(ownerObjectClass, ownerAttributes); }
            catch (Exception ignored) { }
         if (interactionPublished && ownerJoined && ownerInteraction != null) try {
            owner.unpublishInteractionClass(ownerInteraction);
         } catch (Exception ignored) { }
         deleteRegion(owner, subscriptionRegion, ownerJoined);
         deleteRegion(owner, objectRegion, ownerJoined);
         deleteRegion(owner, wrongContextRegion, ownerJoined);
         deleteRegion(owner, uncommittedRegion, ownerJoined);
         deleteRegion(foreign, foreignRegion, foreignJoined);
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (foreignJoined) try { foreign.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
         if (foreignConnected) try { foreign.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      RegionHandle region = ambassador.createRegion(dimensionSet(ambassador, first, second));
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static DimensionHandleSet dimensionSet(RTIambassador ambassador,
         DimensionHandle... dimensions) throws Exception {
      DimensionHandleSet result = ambassador.getDimensionHandleSetFactory().create();
      for (DimensionHandle dimension : dimensions) result.add(dimension);
      return result;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle... regions)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      for (RegionHandle region : regions) result.add(region);
      return result;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regionSet(ambassador, region)));
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
               if ("toString".equals(method.getName())) return "Java TCK regional boundary callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }
}
