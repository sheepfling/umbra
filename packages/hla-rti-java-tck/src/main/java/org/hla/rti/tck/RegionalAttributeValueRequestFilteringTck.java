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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Filters regional Request Attribute Value Update calls using source-region scope. */
final class RegionalAttributeValueRequestFilteringTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] REQUEST_TAG = new byte[] {(byte) 0x91, 0x25};

   private RegionalAttributeValueRequestFilteringTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, String unrelatedDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-regional-value-request-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false, published = false;
      boolean ownerRegionCreated = false, requestRegionCreated = false;
      boolean unrelatedRegionCreated = false, uncommittedRegionCreated = false;
      ObjectClassHandle ownerClass = null, requesterClass = null;
      AttributeHandle ownerAttribute = null, requesterAttribute = null;
      AttributeHandleSet ownerAttributes = null, requesterAttributes = null;
      AttributeSetRegionSetPairList ownerPair = null;
      RegionHandle ownerRegion = null, requestRegion = null;
      RegionHandle unrelatedRegion = null, uncommittedRegion = null;
      ObjectInstanceHandle regionalObject = null, defaultObject = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-regional-value-request-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-regional-value-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle requesterFirst = requester.getDimensionHandle(firstDimensionName);
         DimensionHandle requesterSecond = requester.getDimensionHandle(secondDimensionName);
         DimensionHandle unrelatedDimension = requester.getDimensionHandle(unrelatedDimensionName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute)
               && ownerFirst.equals(requesterFirst) && ownerSecond.equals(requesterSecond),
            "adapter DDM FOM returned inconsistent class, attribute, or dimension handles");
         java.util.Set<DimensionHandle> available =
            requester.getAvailableDimensionsForObjectClass(requesterClass);
         JavaTckSupport.check(available.contains(requesterFirst)
               && available.contains(requesterSecond)
               && !available.contains(unrelatedDimension)
               && !requesterFirst.equals(requesterSecond)
               && !requesterFirst.equals(unrelatedDimension)
               && !requesterSecond.equals(unrelatedDimension)
               && requester.getDimensionUpperBound(requesterFirst) >= 3
               && requester.getDimensionUpperBound(requesterSecond) >= 3
               && requester.getDimensionUpperBound(unrelatedDimension) >= 1,
            "adapter FOM must provide two object dimensions and a distinct unrelated dimension");

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         ownerRegion = createRegion(owner, ownerFirst, ownerSecond, 0, 1);
         ownerRegionCreated = true;
         requestRegion = createRegion(requester, requesterFirst, requesterSecond, 2, 3);
         requestRegionCreated = true;

         DimensionHandleSet unrelatedDimensions = requester.getDimensionHandleSetFactory().create();
         unrelatedDimensions.add(unrelatedDimension);
         unrelatedRegion = requester.createRegion(unrelatedDimensions);
         unrelatedRegionCreated = true;
         setAndCommit(requester, unrelatedRegion, unrelatedDimension, 0, 1);
         DimensionHandleSet requestDimensions = requester.getDimensionHandleSetFactory().create();
         requestDimensions.add(requesterFirst);
         requestDimensions.add(requesterSecond);
         uncommittedRegion = requester.createRegion(requestDimensions);
         uncommittedRegionCreated = true;

         ownerPair = association(owner, ownerAttributes, ownerRegion);
         AttributeSetRegionSetPairList disjointPair =
            association(requester, requesterAttributes, requestRegion);
         AttributeSetRegionSetPairList emptyPair = associationWithEmptyRegions(
            requester, requesterAttributes);
         AttributeSetRegionSetPairList foreignPair =
            association(requester, requesterAttributes, ownerRegion);
         AttributeSetRegionSetPairList wrongContextPair =
            association(requester, requesterAttributes, unrelatedRegion);
         AttributeSetRegionSetPairList uncommittedPair =
            association(requester, requesterAttributes, uncommittedRegion);
         ObjectClassHandle requestClassHandle = requesterClass;

         regionalObject = owner.registerObjectInstanceWithRegions(ownerClass, ownerPair);
         defaultObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(regionalObject != null && regionalObject.encodedLength() > 0
               && defaultObject != null && defaultObject.encodedLength() > 0,
            "regional request filtering registration returned an invalid object handle");

         JavaTckSupport.expectFailure(() -> requester.requestAttributeValueUpdateWithRegions(
               requestClassHandle, foreignPair, REQUEST_TAG.clone()),
            "RegionNotCreatedByThisFederate", "regional request using another federate's region");
         JavaTckSupport.expectFailure(() -> requester.requestAttributeValueUpdateWithRegions(
               requestClassHandle, wrongContextPair, REQUEST_TAG.clone()),
            "InvalidRegionContext", "regional request using an incompatible region context");
         JavaTckSupport.expectFailure(() -> requester.requestAttributeValueUpdateWithRegions(
               requestClassHandle, uncommittedPair, REQUEST_TAG.clone()),
            "InvalidRegion", "regional request using an uncommitted region");

         ownerCallbacks.provided.clear();
         requester.requestAttributeValueUpdateWithRegions(
            requesterClass, emptyPair, REQUEST_TAG.clone());
         for (int pass = 0; pass < 8; ++pass) pump(owner, callbackModel);
         JavaTckSupport.check(ownerCallbacks.provided.isEmpty(),
            "empty regional request set generated a provideAttributeValueUpdate callback");

         requester.requestAttributeValueUpdateWithRegions(
            requesterClass, disjointPair, REQUEST_TAG.clone());
         awaitOwner(owner, callbackModel, () -> ownerCallbacks.provided.size() >= 1,
            "disjoint regional value request callback");
         JavaTckSupport.check(ownerCallbacks.provided.size() == 1
               && defaultObject.equals(ownerCallbacks.provided.get(0).object)
               && ownerAttributes.equals(ownerCallbacks.provided.get(0).attributes)
               && Arrays.equals(REQUEST_TAG, ownerCallbacks.provided.get(0).tag),
            "disjoint request did not select only the default-region object with the request tag");

         setBothAndCommit(requester, requestRegion, requesterFirst, requesterSecond, 0, 1);
         requester.requestAttributeValueUpdateWithRegions(
            requesterClass, disjointPair, REQUEST_TAG.clone());
         awaitOwner(owner, callbackModel, () -> ownerCallbacks.provided.size() >= 3,
            "overlapping regional value request callbacks");
         JavaTckSupport.check(ownerCallbacks.provided.size() == 3,
            "overlapping regional request returned an unexpected callback count");
         boolean sawRegional = false, sawDefault = false;
         for (ProvidedUpdate update : ownerCallbacks.provided) {
            JavaTckSupport.check(ownerAttributes.equals(update.attributes)
                  && Arrays.equals(REQUEST_TAG, update.tag),
               "regional request callback changed requested attributes or tag");
            sawRegional |= regionalObject.equals(update.object);
            sawDefault |= defaultObject.equals(update.object);
         }
         JavaTckSupport.check(sawRegional && sawDefault,
            "overlapping regional request omitted a source object");

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            requester.requestAttributeValueUpdateWithRegions(
               requesterClass, disjointPair, REQUEST_TAG.clone());
            setBothAndCommit(requester, requestRegion, requesterFirst, requesterSecond, 2, 3);
            awaitOwner(owner, callbackModel, () -> ownerCallbacks.provided.size() >= 4,
               "evoked regional value-request re-evaluation");
            JavaTckSupport.check(ownerCallbacks.provided.size() == 4
                  && defaultObject.equals(ownerCallbacks.provided.get(3).object),
               "evoked request did not suppress its now-disjoint explicit source object");
         } else {
            requester.requestAttributeValueUpdateWithRegions(
               requesterClass, disjointPair, REQUEST_TAG.clone());
            awaitOwner(owner, callbackModel, () -> ownerCallbacks.provided.size() >= 5,
               "immediate regional value request callbacks");
            setBothAndCommit(requester, requestRegion, requesterFirst, requesterSecond, 2, 3);
            pump(owner, callbackModel);
            JavaTckSupport.check(ownerCallbacks.provided.size() == 5,
               "immediate regional value request replayed callbacks after region movement");
         }

         requester.deleteRegion(unrelatedRegion);
         unrelatedRegionCreated = false;
         requester.deleteRegion(uncommittedRegion);
         uncommittedRegionCreated = false;
         requester.deleteRegion(requestRegion);
         requestRegionCreated = false;
         owner.unassociateRegionsForUpdates(regionalObject, ownerPair);
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         published = false;
         owner.deleteRegion(ownerRegion);
         ownerRegionCreated = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (ownerJoined && regionalObject != null && ownerPair != null) try {
            owner.unassociateRegionsForUpdates(regionalObject, ownerPair);
         } catch (Exception ignored) { }
         if (published && ownerJoined) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         deleteRegion(requester, unrelatedRegion, unrelatedRegionCreated, requesterJoined);
         deleteRegion(requester, uncommittedRegion, uncommittedRegionCreated, requesterJoined);
         deleteRegion(requester, requestRegion, requestRegionCreated, requesterJoined);
         deleteRegion(owner, ownerRegion, ownerRegionCreated, ownerJoined);
         if (requesterJoined) try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      setRange(ambassador, region, first, lower, upper);
      setRange(ambassador, region, second, lower, upper);
      commit(ambassador, region);
      return region;
   }

   private static void setAndCommit(RTIambassador ambassador, RegionHandle region,
         DimensionHandle dimension, long lower, long upper) throws Exception {
      setRange(ambassador, region, dimension, lower, upper);
      commit(ambassador, region);
   }

   private static void setBothAndCommit(RTIambassador ambassador, RegionHandle region,
         DimensionHandle first, DimensionHandle second, long lower, long upper) throws Exception {
      setRange(ambassador, region, first, lower, upper);
      setRange(ambassador, region, second, lower, upper);
      commit(ambassador, region);
   }

   private static void setRange(RTIambassador ambassador, RegionHandle region,
         DimensionHandle dimension, long lower, long upper) throws Exception {
      ambassador.setRangeBounds(region, dimension, new hla.rti1516_2025.RangeBounds(lower, upper));
   }

   private static void commit(RTIambassador ambassador, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      return association(ambassador, attributes, regions);
   }

   private static AttributeSetRegionSetPairList associationWithEmptyRegions(
         RTIambassador ambassador, AttributeHandleSet attributes) throws Exception {
      return association(ambassador, attributes, ambassador.getRegionHandleSetFactory().create());
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandleSet regions) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions));
      return result;
   }

   private static void awaitOwner(RTIambassador owner, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(owner, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean created, boolean joined) {
      if (created && joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static final class ProvidedUpdate {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private ProvidedUpdate(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag;
      }
   }

   private static final class Recorder {
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("provideAttributeValueUpdate".equals(name) && arguments != null
                     && arguments.length == 3) {
                  provided.add(new ProvidedUpdate((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) ((AttributeHandleSet) arguments[1]).clone(),
                     ((byte[]) arguments[2]).clone()));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional value-request callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
