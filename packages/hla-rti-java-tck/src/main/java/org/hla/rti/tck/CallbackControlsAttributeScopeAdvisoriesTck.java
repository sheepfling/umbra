package org.hla.rti.tck;

import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

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

/** Tests callback gating for standard DDM attributes-in/out-of-scope advisories. */
final class CallbackControlsAttributeScopeAdvisoriesTck {
   private CallbackControlsAttributeScopeAdvisoriesTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      String federation = "java-tck-callback-attribute-scope-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean registered = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle memberClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle memberAttribute = null;
      ObjectInstanceHandle object = null;
      RegionHandle sourceRegion = null;
      RegionHandle alternateSourceRegion = null;
      RegionHandle memberRegion = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet memberAttributes = null;
      AttributeSetRegionSetPairList sourcePair = null;
      AttributeSetRegionSetPairList alternateSourcePair = null;
      AttributeSetRegionSetPairList memberPair = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberRecorder.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-scope-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-scope-member", federation);
         memberJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         memberClass = member.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle memberFirst = member.getDimensionHandle(firstDimensionName);
         DimensionHandle memberSecond = member.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(memberClass)
               && ownerAttribute.equals(memberAttribute)
               && ownerFirst.equals(memberFirst) && ownerSecond.equals(memberSecond),
            "adapter FOM did not resolve stable object, attribute, and dimension handles");
         JavaTckSupport.check(owner.getAvailableDimensionsForObjectClass(ownerClass)
               .contains(ownerFirst)
               && owner.getAvailableDimensionsForObjectClass(ownerClass).contains(ownerSecond)
               && member.getAvailableDimensionsForObjectClass(memberClass).contains(memberFirst)
               && member.getAvailableDimensionsForObjectClass(memberClass).contains(memberSecond),
            "adapter object class does not expose both configured dimensions");

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;

         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 1, 5);
         alternateSourceRegion = createRegion(owner, ownerFirst, ownerSecond, 7, 9, 7, 9);
         memberRegion = createRegion(member, memberFirst, memberSecond, 2, 6, 2, 6);
         sourcePair = association(owner, ownerAttributes, sourceRegion);
         alternateSourcePair = association(owner, ownerAttributes, alternateSourceRegion);
         memberPair = association(member, memberAttributes, memberRegion);

         member.setAttributeScopeAdvisorySwitch(false);
         JavaTckSupport.check(!member.getAttributeScopeAdvisorySwitch(),
            "attribute-scope advisory switch did not turn off");
         member.setAttributeScopeAdvisorySwitch(true);
         JavaTckSupport.check(member.getAttributeScopeAdvisorySwitch(),
            "attribute-scope advisory switch did not turn on");
         member.subscribeObjectClassAttributesWithRegions(memberClass, memberPair);
         subscribed = true;
         ObjectInstanceHandle registeredObject =
            owner.registerObjectInstanceWithRegions(ownerClass, sourcePair);
         object = registeredObject;
         registered = true;
         await(owner, member, callbackModel,
            () -> memberRecorder.discoveries.contains(registeredObject),
            objectClassName, "regional object discovery");

         serviceMemberCallbacks(member, callbackModel, objectClassName);
         memberRecorder.clearScopeAdvisories();

         moveOutOfScope(owner, object, sourcePair, alternateSourcePair);
         await(owner, member, callbackModel,
            () -> !memberRecorder.attributesOutOfScope.isEmpty(), objectClassName,
            "baseline attributes-out-of-scope advisory");
         requireSingleScopeAdvisory(memberRecorder.attributesOutOfScope, object,
            memberAttributes, "baseline attributes-out-of-scope advisory");
         JavaTckSupport.check(memberRecorder.attributesInScope.isEmpty(),
            "baseline out-of-scope transition also produced an in-scope advisory");
         memberRecorder.clearScopeAdvisories();

         moveIntoScope(owner, object, sourcePair, alternateSourcePair);
         await(owner, member, callbackModel,
            () -> !memberRecorder.attributesInScope.isEmpty(), objectClassName,
            "baseline attributes-in-scope advisory");
         requireSingleScopeAdvisory(memberRecorder.attributesInScope, object,
            memberAttributes, "baseline attributes-in-scope advisory");
         JavaTckSupport.check(memberRecorder.attributesOutOfScope.isEmpty(),
            "baseline in-scope transition also produced an out-of-scope advisory");
         memberRecorder.clearScopeAdvisories();

         member.disableCallbacks();
         moveOutOfScope(owner, object, sourcePair, alternateSourcePair);
         serviceMemberCallbacks(member, callbackModel, objectClassName);
         JavaTckSupport.check(memberRecorder.attributesInScope.isEmpty()
               && memberRecorder.attributesOutOfScope.isEmpty(),
            "disabled callbacks exposed an out-of-scope advisory");
         member.enableCallbacks();
         serviceMemberCallbacks(member, callbackModel, objectClassName);
         await(owner, member, callbackModel,
            () -> !memberRecorder.attributesOutOfScope.isEmpty(), objectClassName,
            "re-enabled attributes-out-of-scope advisory");
         requireSingleScopeAdvisory(memberRecorder.attributesOutOfScope, object,
            memberAttributes, "re-enabled attributes-out-of-scope advisory");
         JavaTckSupport.check(memberRecorder.attributesInScope.isEmpty(),
            "re-enabled out-of-scope transition also produced an in-scope advisory");
         memberRecorder.clearScopeAdvisories();

         member.disableCallbacks();
         moveIntoScope(owner, object, sourcePair, alternateSourcePair);
         serviceMemberCallbacks(member, callbackModel, objectClassName);
         JavaTckSupport.check(memberRecorder.attributesInScope.isEmpty()
               && memberRecorder.attributesOutOfScope.isEmpty(),
            "disabled callbacks exposed an in-scope advisory");
         member.enableCallbacks();
         serviceMemberCallbacks(member, callbackModel, objectClassName);
         await(owner, member, callbackModel,
            () -> !memberRecorder.attributesInScope.isEmpty(), objectClassName,
            "re-enabled attributes-in-scope advisory");
         requireSingleScopeAdvisory(memberRecorder.attributesInScope, object,
            memberAttributes, "re-enabled attributes-in-scope advisory");
         JavaTckSupport.check(memberRecorder.attributesOutOfScope.isEmpty(),
            "re-enabled in-scope transition also produced an out-of-scope advisory");

         member.unsubscribeObjectClassAttributesWithRegions(memberClass, memberPair);
         subscribed = false;
         owner.unassociateRegionsForUpdates(object, sourcePair);
         registered = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         published = false;
         owner.deleteRegion(sourceRegion);
         sourceRegion = null;
         owner.deleteRegion(alternateSourceRegion);
         alternateSourceRegion = null;
         member.deleteRegion(memberRegion);
         memberRegion = null;
      } finally {
         if (memberJoined) {
            try { member.enableCallbacks(); } catch (Exception ignored) { }
         }
         if (subscribed && memberJoined && memberClass != null && memberPair != null) {
            try { member.unsubscribeObjectClassAttributesWithRegions(memberClass, memberPair); }
            catch (Exception ignored) { }
         }
         if (registered && ownerJoined && object != null) {
            if (sourcePair != null) {
               try { owner.unassociateRegionsForUpdates(object, sourcePair); }
               catch (Exception ignored) { }
            }
            if (alternateSourcePair != null) {
               try { owner.unassociateRegionsForUpdates(object, alternateSourcePair); }
               catch (Exception ignored) { }
            }
         }
         if (published && ownerJoined && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (sourceRegion != null && ownerJoined) {
            try { owner.deleteRegion(sourceRegion); } catch (Exception ignored) { }
         }
         if (alternateSourceRegion != null && ownerJoined) {
            try { owner.deleteRegion(alternateSourceRegion); } catch (Exception ignored) { }
         }
         if (memberRegion != null && memberJoined) {
            try { member.deleteRegion(memberRegion); } catch (Exception ignored) { }
         }
         if (memberJoined) resign(member, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador,
         DimensionHandle firstDimension, DimensionHandle secondDimension,
         long firstLower, long firstUpper, long secondLower, long secondUpper) throws Exception {
      JavaTckSupport.check(ambassador.getDimensionUpperBound(firstDimension) >= firstUpper
            && ambassador.getDimensionUpperBound(secondDimension) >= secondUpper,
         "adapter DDM dimension bounds do not contain the configured scope ranges");
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(firstDimension);
      dimensions.add(secondDimension);
      RegionHandle region = ambassador.createRegion(dimensions);
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.setRangeBounds(region, firstDimension,
         new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, secondDimension,
         new RangeBounds(secondLower, secondUpper));
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      AttributeSetRegionSetPairList associations =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      associations.add(new AttributeRegionAssociation(attributes, regions));
      return associations;
   }

   private static void moveOutOfScope(RTIambassador owner, ObjectInstanceHandle object,
         AttributeSetRegionSetPairList sourcePair,
         AttributeSetRegionSetPairList alternateSourcePair) throws Exception {
      owner.associateRegionsForUpdates(object, alternateSourcePair);
      owner.unassociateRegionsForUpdates(object, sourcePair);
   }

   private static void moveIntoScope(RTIambassador owner, ObjectInstanceHandle object,
         AttributeSetRegionSetPairList sourcePair,
         AttributeSetRegionSetPairList alternateSourcePair) throws Exception {
      owner.associateRegionsForUpdates(object, sourcePair);
      owner.unassociateRegionsForUpdates(object, alternateSourcePair);
   }

   private static void requireSingleScopeAdvisory(List<ScopeAdvisory> reports,
         ObjectInstanceHandle object, AttributeHandleSet attributes, String description) {
      JavaTckSupport.check(reports.size() == 1,
         description + " returned an unexpected callback count");
      ScopeAdvisory report = reports.get(0);
      JavaTckSupport.check(object.equals(report.object) && attributes.equals(report.attributes),
         description + " returned the wrong object or attribute set");
   }

   private static void serviceMemberCallbacks(RTIambassador member,
         CallbackModel callbackModel, String objectClassName) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         while (member.evokeMultipleCallbacks(0.0, 0.0)) {
            // Drain callbacks queued while callbacks were disabled or just enabled.
         }
      } else {
         member.getObjectClassHandle(objectClassName);
      }
   }

   private static void await(RTIambassador owner, RTIambassador member,
         CallbackModel callbackModel, java.util.function.BooleanSupplier condition,
         String objectClassName, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         serviceMemberCallbacks(member, callbackModel, objectClassName);
         if (callbackModel == CallbackModel.HLA_EVOKED) owner.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class ScopeAdvisory {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;

      private ScopeAdvisory(ObjectInstanceHandle object, AttributeHandleSet attributes) {
         this.object = object;
         this.attributes = attributes;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ScopeAdvisory> attributesInScope = new CopyOnWriteArrayList<>();
      private final List<ScopeAdvisory> attributesOutOfScope = new CopyOnWriteArrayList<>();

      private void clearScopeAdvisories() {
         attributesInScope.clear();
         attributesOutOfScope.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributesInScope".equals(name)) {
                  attributesInScope.add(new ScopeAdvisory(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1]));
               } else if ("attributesOutOfScope".equals(name)) {
                  attributesOutOfScope.add(new ScopeAdvisory(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1]));
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute-scope callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
