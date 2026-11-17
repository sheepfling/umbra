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
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import java.lang.reflect.Proxy;
import java.util.Set;
import java.util.UUID;

/** Exercises region services across membership boundaries and live region lifecycle. */
final class RegionLifecycleTck {
   private RegionLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador lifecycle = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-region-lifecycle-" + UUID.randomUUID();
      boolean lifecycleConnected = false, ownerConnected = false, memberConnected = false;
      boolean created = false, ownerJoined = false, memberJoined = false;
      boolean ownerPublished = false;
      RegionHandle ownerRegion = null, memberRegion = null, oneDimensionRegion = null;
      AttributeHandleSet ownerAttributes = null;
      ObjectInstanceHandle boundaryObject = null;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-region-lifecycle-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-region-lifecycle-member", federation);
         memberJoined = true;

         ObjectClassHandle ownerObjectClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberObjectClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerObjectClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberObjectClass, attributeName);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle memberInteraction =
            member.getInteractionClassHandle(interactionClassName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle memberParameter = member.getParameterHandle(memberInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle memberFirst = member.getDimensionHandle(firstDimensionName);
         DimensionHandle memberSecond = member.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerObjectClass != null && ownerObjectClass.encodedLength() > 0
               && memberObjectClass != null && memberObjectClass.encodedLength() > 0
               && ownerAttribute != null && ownerAttribute.encodedLength() > 0
               && memberAttribute != null && memberAttribute.encodedLength() > 0
               && ownerInteraction != null && ownerInteraction.encodedLength() > 0
               && memberInteraction != null && memberInteraction.encodedLength() > 0
               && ownerParameter != null && ownerParameter.encodedLength() > 0
               && memberParameter != null && memberParameter.encodedLength() > 0
               && ownerFirst != null && ownerFirst.encodedLength() > 0
               && ownerSecond != null && ownerSecond.encodedLength() > 0
               && memberFirst != null && memberFirst.encodedLength() > 0
               && memberSecond != null && memberSecond.encodedLength() > 0
               && !ownerFirst.equals(ownerSecond) && !memberFirst.equals(memberSecond),
            "adapter DDM lookups returned invalid or duplicate handles");
         JavaTckSupport.check(ownerObjectClass.equals(memberObjectClass)
               && ownerAttribute.equals(memberAttribute)
               && ownerInteraction.equals(memberInteraction)
               && ownerParameter.equals(memberParameter)
               && ownerFirst.equals(memberFirst) && ownerSecond.equals(memberSecond),
            "adapter FOM handles changed across region-lifecycle federates");
         JavaTckSupport.check(firstDimensionName.equals(owner.getDimensionName(ownerFirst))
               && secondDimensionName.equals(owner.getDimensionName(ownerSecond))
               && firstDimensionName.equals(member.getDimensionName(memberFirst))
               && secondDimensionName.equals(member.getDimensionName(memberSecond)),
            "region-lifecycle dimension handles did not resolve to adapter-supplied names");
         DimensionHandleSet ownerObjectDimensions =
            owner.getAvailableDimensionsForObjectClass(ownerObjectClass);
         DimensionHandleSet ownerInteractionDimensions =
            owner.getAvailableDimensionsForInteractionClass(ownerInteraction);
         DimensionHandleSet memberObjectDimensions =
            member.getAvailableDimensionsForObjectClass(memberObjectClass);
         DimensionHandleSet memberInteractionDimensions =
            member.getAvailableDimensionsForInteractionClass(memberInteraction);
         JavaTckSupport.check(ownerObjectDimensions.contains(ownerFirst)
               && ownerObjectDimensions.contains(ownerSecond)
               && ownerInteractionDimensions.contains(ownerFirst)
               && ownerInteractionDimensions.contains(ownerSecond)
               && memberObjectDimensions.contains(memberFirst)
               && memberObjectDimensions.contains(memberSecond)
               && memberInteractionDimensions.contains(memberFirst)
               && memberInteractionDimensions.contains(memberSecond),
            "adapter classes must expose both configured DDM dimensions");
         long firstBound = owner.getDimensionUpperBound(ownerFirst);
         long secondBound = owner.getDimensionUpperBound(ownerSecond);
         JavaTckSupport.check(firstBound >= 7 && secondBound >= 7
               && firstBound == member.getDimensionUpperBound(memberFirst)
               && secondBound == member.getDimensionUpperBound(memberSecond),
            "region-lifecycle federates reported inconsistent dimension bounds");

         ownerRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 2, 7);
         memberRegion = createRegion(member, memberFirst, memberSecond, 1, 5, 2, 7);
         assertRegion(owner, ownerRegion, ownerFirst, ownerSecond, 1, 5, 2, 7,
            "owner region initial metadata");
         assertRegion(member, memberRegion, memberFirst, memberSecond, 1, 5, 2, 7,
            "member region initial metadata");

         ownerAttributes = owner.getAttributeHandleSetFactory().create();
         ownerAttributes.add(ownerAttribute);
         owner.publishObjectClassAttributes(ownerObjectClass, ownerAttributes);
         ownerPublished = true;
         boundaryObject = owner.registerObjectInstance(ownerObjectClass);
         JavaTckSupport.check(boundaryObject != null && boundaryObject.encodedLength() > 0,
            "region-lifecycle boundary setup returned an invalid object handle");

         BoundaryFixture fixture = BoundaryFixture.create(owner, ownerObjectClass,
            ownerInteraction, boundaryObject, ownerFirst, ownerRegion);
         fixture.expect(lifecycle, "NotConnected", "before connect");
         lifecycle.connect(callbacks(), callbackModel);
         lifecycleConnected = true;
         fixture.expect(lifecycle, "FederateNotExecutionMember", "before joining an execution");
         lifecycle.disconnect();
         lifecycleConnected = false;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         LogicalTime<?, ?> initialTime = (LogicalTime<?, ?>) timeFactory.makeInitial();
         ParameterHandleValueMap emptyParameters =
            owner.getParameterHandleValueMapFactory().create(1);
         fixture.expectTimestampedInteraction(lifecycle, initialTime, emptyParameters,
            "NotConnected", "before connect");
         lifecycle.connect(callbacks(), callbackModel);
         lifecycleConnected = true;
         fixture.expectTimestampedInteraction(lifecycle, initialTime, emptyParameters,
            "FederateNotExecutionMember", "before joining an execution");
         lifecycle.disconnect();
         lifecycleConnected = false;

         owner.setRangeBounds(ownerRegion, ownerFirst, new RangeBounds(0, 4));
         RegionHandleSet ownerRegions = regionSet(owner, ownerRegion);
         owner.commitRegionModifications(ownerRegions);
         assertRegion(owner, ownerRegion, ownerFirst, ownerSecond, 0, 4, 2, 7,
            "committed region modification");
         RegionHandle modifiedRegion = ownerRegion;

         oneDimensionRegion = createRegion(owner, ownerFirst);
         RegionHandle oneDimension = oneDimensionRegion;
         JavaTckSupport.expectFailure(() -> owner.getRangeBounds(oneDimension, ownerSecond),
            "RegionDoesNotContainSpecifiedDimension", "reading an absent region dimension");
         JavaTckSupport.expectFailure(() -> owner.setRangeBounds(oneDimension, ownerSecond,
               new RangeBounds(0, 1)),
            "RegionDoesNotContainSpecifiedDimension", "setting an absent region dimension");
         owner.deleteRegion(oneDimensionRegion);
         oneDimensionRegion = null;

         JavaTckSupport.expectFailure(() -> owner.setRangeBounds(modifiedRegion, ownerFirst,
               new RangeBounds(4, 4)),
            "InvalidRangeBound", "using equal lower and upper region bounds");
         JavaTckSupport.expectFailure(() -> owner.setRangeBounds(modifiedRegion, ownerSecond,
               new RangeBounds(0, owner.getDimensionUpperBound(ownerSecond) + 1)),
            "InvalidRangeBound", "setting a range above the dimension upper bound");

         fixture.deletedRegion = ownerRegion;
         owner.deleteRegion(ownerRegion);
         ownerRegion = null;
         member.deleteRegion(memberRegion);
         memberRegion = null;
         RegionHandle deletedOwnerRegion = fixture.deletedRegion;
         JavaTckSupport.expectFailure(() -> owner.getDimensionHandleSet(deletedOwnerRegion),
            "InvalidRegion", "looking up dimensions after region deletion");

         owner.deleteObjectInstance(boundaryObject, new byte[0]);
         boundaryObject = null;
         owner.unpublishObjectClassAttributes(ownerObjectClass, ownerAttributes);
         ownerPublished = false;

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (lifecycleConnected) try { lifecycle.disconnect(); } catch (Exception ignored) { }
         if (ownerJoined && boundaryObject != null) try {
            owner.deleteObjectInstance(boundaryObject, new byte[0]);
         } catch (Exception ignored) { }
         if (ownerJoined && ownerPublished && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(owner.getObjectClassHandle(objectClassName),
               ownerAttributes);
         } catch (Exception ignored) { }
         deleteRegion(owner, oneDimensionRegion, ownerJoined);
         deleteRegion(owner, ownerRegion, ownerJoined);
         deleteRegion(member, memberRegion, memberJoined);
         if (ownerJoined) try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (memberJoined) try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle... dimensions)
         throws Exception {
      DimensionHandleSet dimensionSet = ambassador.getDimensionHandleSetFactory().create();
      for (DimensionHandle dimension : dimensions) dimensionSet.add(dimension);
      RegionHandle region = ambassador.createRegion(dimensionSet);
      return region;
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long firstLower, long firstUpper, long secondLower,
         long secondUpper) throws Exception {
      RegionHandle region = createRegion(ambassador, first, second);
      ambassador.setRangeBounds(region, first, new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, second, new RangeBounds(secondLower, secondUpper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      return regions;
   }

   private static void assertRegion(RTIambassador ambassador, RegionHandle region,
         DimensionHandle first, DimensionHandle second, long firstLower, long firstUpper,
         long secondLower, long secondUpper, String description) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSet(region);
      JavaTckSupport.check(dimensions.size() == 2 && dimensions.contains(first)
            && dimensions.contains(second), description + " exposed incorrect dimensions");
      JavaTckSupport.check(new RangeBounds(firstLower, firstUpper)
            .equals(ambassador.getRangeBounds(region, first))
            && new RangeBounds(secondLower, secondUpper)
               .equals(ambassador.getRangeBounds(region, second)),
         description + " returned incorrect range bounds");
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region, boolean joined) {
      if (joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK region lifecycle callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static final class BoundaryFixture {
      private final DimensionHandleSet emptyDimensions;
      private final RegionHandleSet emptyRegions;
      private final AttributeSetRegionSetPairList emptyAttributeRegionPairs;
      private final ParameterHandleValueMap emptyParameters;
      private final ObjectClassHandle objectClass;
      private final InteractionClassHandle interactionClass;
      private final ObjectInstanceHandle objectInstance;
      private final DimensionHandle dimension;
      private final RegionHandle region;
      private RegionHandle deletedRegion;

      private BoundaryFixture(RTIambassador owner, ObjectClassHandle objectClass,
            InteractionClassHandle interactionClass, ObjectInstanceHandle objectInstance,
            DimensionHandle dimension, RegionHandle region) throws Exception {
         emptyDimensions = owner.getDimensionHandleSetFactory().create();
         emptyRegions = owner.getRegionHandleSetFactory().create();
         emptyAttributeRegionPairs =
            owner.getAttributeSetRegionSetPairListFactory().create(1);
         emptyParameters = owner.getParameterHandleValueMapFactory().create(1);
         this.objectClass = objectClass;
         this.interactionClass = interactionClass;
         this.objectInstance = objectInstance;
         this.dimension = dimension;
         this.region = region;
      }

      private static BoundaryFixture create(RTIambassador owner, ObjectClassHandle objectClass,
            InteractionClassHandle interactionClass, ObjectInstanceHandle objectInstance,
            DimensionHandle dimension, RegionHandle region) throws Exception {
         return new BoundaryFixture(owner, objectClass, interactionClass, objectInstance,
            dimension, region);
      }

      private void expect(RTIambassador ambassador, String exception, String phase)
            throws Exception {
         RangeBounds range = new RangeBounds(0, 1);
         JavaTckSupport.expectFailure(() -> ambassador.createRegion(emptyDimensions), exception,
            "creating an empty region " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.commitRegionModifications(emptyRegions),
            exception, "committing an empty region set " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.deleteRegion(region), exception,
            "deleting another federate's region " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.registerObjectInstanceWithRegions(
               objectClass, emptyAttributeRegionPairs), exception,
            "registering an object with regions " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.registerObjectInstanceWithRegions(
               objectClass, emptyAttributeRegionPairs, "tck-region-lifecycle-object"),
            exception, "named regional object registration " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.associateRegionsForUpdates(
               objectInstance, emptyAttributeRegionPairs), exception,
            "associating regions " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.unassociateRegionsForUpdates(
               objectInstance, emptyAttributeRegionPairs), exception,
            "unassociating regions " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.subscribeObjectClassAttributesWithRegions(
               objectClass, emptyAttributeRegionPairs), exception,
            "subscribing to regional attributes " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.unsubscribeObjectClassAttributesWithRegions(
               objectClass, emptyAttributeRegionPairs), exception,
            "unsubscribing from regional attributes " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.subscribeInteractionClassWithRegions(
               interactionClass, emptyRegions), exception,
            "subscribing to regional interactions " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.unsubscribeInteractionClassWithRegions(
               interactionClass, emptyRegions), exception,
            "unsubscribing from regional interactions " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.sendInteractionWithRegions(
               interactionClass, emptyParameters, emptyRegions, new byte[0]), exception,
            "sending a regional interaction " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.requestAttributeValueUpdateWithRegions(
               objectClass, emptyAttributeRegionPairs, new byte[0]), exception,
            "requesting regional attribute values " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.getRangeBounds(region, dimension),
            exception, "reading bounds for another federate's region " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.setRangeBounds(
               region, dimension, range), exception,
            "setting bounds for another federate's region " + phase);
         JavaTckSupport.expectFailure(() -> ambassador.getDimensionHandleSet(region),
            exception, "reading dimensions for another federate's region " + phase);
      }

      private void expectTimestampedInteraction(RTIambassador ambassador,
            LogicalTime<?, ?> time, ParameterHandleValueMap parameters, String exception,
            String phase) throws Exception {
         JavaTckSupport.expectFailure(() -> ambassador.sendInteractionWithRegions(
               interactionClass, parameters, emptyRegions, new byte[0], time), exception,
            "sending a timestamped regional interaction " + phase);
      }
   }
}
