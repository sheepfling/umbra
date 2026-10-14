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
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies standard attribute scope transitions for regional DDM declarations. */
final class AttributeScopeAdvisoriesTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private AttributeScopeAdvisoriesTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder memberCallbacks = new Recorder();
      String federation = "java-tck-attribute-scope-advisories-" + UUID.randomUUID();
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
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet memberAttributes = null;
      ObjectInstanceHandle object = null;
      RegionHandle sourceRegion = null;
      RegionHandle alternateSourceRegion = null;
      RegionHandle memberRegion = null;
      AttributeSetRegionSetPairList sourcePair = null;
      AttributeSetRegionSetPairList alternateSourcePair = null;
      AttributeSetRegionSetPairList memberPair = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberCallbacks.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("attribute-scope-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("attribute-scope-member", federation);
         memberJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle memberFirst = member.getDimensionHandle(firstDimensionName);
         DimensionHandle memberSecond = member.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(memberClass) && ownerAttribute.equals(memberAttribute)
               && ownerFirst.equals(memberFirst) && ownerSecond.equals(memberSecond),
            "adapter DDM FOM resolved inconsistent object, attribute, or dimension handles");
         checkDimensions(owner, ownerClass, ownerFirst, ownerSecond,
            firstDimensionName, secondDimensionName);
         checkDimensions(member, memberClass, memberFirst, memberSecond,
            firstDimensionName, secondDimensionName);

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
         object = owner.registerObjectInstanceWithRegions(ownerClass, sourcePair);
         registered = true;
         ObjectInstanceHandle registeredObject = object;
         await(member, callbackModel,
            () -> memberCallbacks.discoveries.contains(registeredObject),
            "regional scope object discovery");

         settle(member, callbackModel);
         memberCallbacks.clearScopeAdvisories();

         moveOutOfScope(owner, object, sourcePair, alternateSourcePair);
         await(member, callbackModel,
            () -> !memberCallbacks.attributesOutOfScope.isEmpty(),
            "attribute out-of-scope advisory");
         requireSingle(memberCallbacks.attributesOutOfScope, object, memberAttributes,
            "attribute out-of-scope advisory");
         JavaTckSupport.check(memberCallbacks.attributesInScope.isEmpty(),
            "source reassociation also produced an in-scope advisory");
         memberCallbacks.clearScopeAdvisories();

         moveIntoScope(owner, object, sourcePair, alternateSourcePair);
         await(member, callbackModel,
            () -> !memberCallbacks.attributesInScope.isEmpty(),
            "attribute in-scope advisory");
         requireSingle(memberCallbacks.attributesInScope, object, memberAttributes,
            "attribute in-scope advisory");
         JavaTckSupport.check(memberCallbacks.attributesOutOfScope.isEmpty(),
            "source reassociation also produced an out-of-scope advisory");
         memberCallbacks.clearScopeAdvisories();

         member.unsubscribeObjectClassAttributesWithRegions(memberClass, memberPair);
         subscribed = false;
         await(member, callbackModel,
            () -> !memberCallbacks.attributesOutOfScope.isEmpty(),
            "regional unsubscribe out-of-scope advisory");
         requireSingle(memberCallbacks.attributesOutOfScope, object, memberAttributes,
            "regional unsubscribe out-of-scope advisory");
         memberCallbacks.clearScopeAdvisories();
         member.subscribeObjectClassAttributesWithRegions(memberClass, memberPair);
         subscribed = true;
         await(member, callbackModel,
            () -> !memberCallbacks.attributesInScope.isEmpty(),
            "regional resubscribe in-scope advisory");
         requireSingle(memberCallbacks.attributesInScope, object, memberAttributes,
            "regional resubscribe in-scope advisory");
         JavaTckSupport.check(memberCallbacks.attributesOutOfScope.isEmpty(),
            "regional resubscribe also produced an out-of-scope advisory");
         memberCallbacks.clearScopeAdvisories();
         member.unsubscribeObjectClassAttributesWithRegions(memberClass, memberPair);
         subscribed = false;
         await(member, callbackModel,
            () -> !memberCallbacks.attributesOutOfScope.isEmpty(),
            "second regional unsubscribe out-of-scope advisory");
         requireSingle(memberCallbacks.attributesOutOfScope, object, memberAttributes,
            "second regional unsubscribe out-of-scope advisory");
         memberCallbacks.clearScopeAdvisories();

         member.setAttributeScopeAdvisorySwitch(false);
         JavaTckSupport.check(!member.getAttributeScopeAdvisorySwitch(),
            "attribute-scope advisory switch did not remain disabled");
         member.subscribeObjectClassAttributesWithRegions(memberClass, memberPair);
         subscribed = true;
         settle(member, callbackModel);
         JavaTckSupport.check(memberCallbacks.attributesInScope.isEmpty()
               && memberCallbacks.attributesOutOfScope.isEmpty(),
            "disabled scope switch did not suppress an in-scope transition");
         member.unsubscribeObjectClassAttributesWithRegions(memberClass, memberPair);
         subscribed = false;
         settle(member, callbackModel);
         JavaTckSupport.check(memberCallbacks.attributesInScope.isEmpty()
               && memberCallbacks.attributesOutOfScope.isEmpty(),
            "disabled scope switch did not suppress an out-of-scope transition");
         member.setAttributeScopeAdvisorySwitch(true);
         JavaTckSupport.check(member.getAttributeScopeAdvisorySwitch(),
            "attribute-scope advisory switch did not restore its enabled state");
         member.subscribeObjectClassAttributesWithRegions(memberClass, memberPair);
         subscribed = true;
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            requireSingle(memberCallbacks.attributesInScope, object, memberAttributes,
               "re-enabled regional subscribe in-scope advisory");
            memberCallbacks.clearScopeAdvisories();
         }
         member.setRangeBounds(memberRegion, memberFirst, new RangeBounds(7, 9));
         member.setRangeBounds(memberRegion, memberSecond, new RangeBounds(7, 9));
         commitRegion(member, memberRegion);
         await(member, callbackModel,
            () -> !memberCallbacks.attributesOutOfScope.isEmpty(),
            "region-change out-of-scope advisory");
         requireSingle(memberCallbacks.attributesOutOfScope, object, memberAttributes,
            "region-change out-of-scope advisory");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(memberCallbacks.attributesInScope.isEmpty(),
               "evoked delivery did not suppress the stale in-scope advisory");
         }

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

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second, String firstName, String secondName)
         throws Exception {
      JavaTckSupport.check(ambassador.getAvailableDimensionsForObjectClass(objectClass)
            .contains(first) && ambassador.getAvailableDimensionsForObjectClass(objectClass)
            .contains(second) && ambassador.getDimensionName(first).equals(firstName)
            && ambassador.getDimensionName(second).equals(secondName)
            && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         "adapter DDM object class omitted a configured dimension or scope range");
   }

   private static RegionHandle createRegion(RTIambassador ambassador,
         DimensionHandle firstDimension, DimensionHandle secondDimension,
         long firstLower, long firstUpper, long secondLower, long secondUpper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(firstDimension);
      dimensions.add(secondDimension);
      RegionHandle region = ambassador.createRegion(dimensions);
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.setRangeBounds(region, firstDimension, new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, secondDimension, new RangeBounds(secondLower, secondUpper));
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static void commitRegion(RTIambassador ambassador, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      pairs.add(new AttributeRegionAssociation(attributes, regions));
      return pairs;
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

   private static void requireSingle(List<ScopeAdvisory> reports, ObjectInstanceHandle object,
         AttributeHandleSet attributes, String description) {
      JavaTckSupport.check(reports.size() == 1, description + " returned an unexpected callback count");
      ScopeAdvisory report = reports.get(0);
      JavaTckSupport.check(object.equals(report.object) && attributes.equals(report.attributes),
         description + " returned the wrong object or attribute set");
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            while (ambassador.evokeMultipleCallbacks(0.0, 0.0)) { }
         } else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         while (ambassador.evokeMultipleCallbacks(0.0, 0.0)) { }
      } else {
         long deadline = System.nanoTime() + 100_000_000L;
         while (System.nanoTime() < deadline) Thread.yield();
      }
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
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributesInScope".equals(name)) {
                  attributesInScope.add(new ScopeAdvisory((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               } else if ("attributesOutOfScope".equals(name)) {
                  attributesOutOfScope.add(new ScopeAdvisory((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute-scope callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
