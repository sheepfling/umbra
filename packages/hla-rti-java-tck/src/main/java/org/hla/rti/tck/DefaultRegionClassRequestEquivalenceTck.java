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
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Compares ordinary and broad explicit-region class attribute-value requests. */
final class DefaultRegionClassRequestEquivalenceTck {
   private static final byte[] ORDINARY_TAG = {(byte) 0xA1, 0x01};
   private static final byte[] REGIONAL_TAG = {(byte) 0xB2, 0x02};
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private DefaultRegionClassRequestEquivalenceTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "default-region class request equivalence has native evidence only for evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador ordinary = factory.getRtiAmbassador();
      RTIambassador regional = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      String federation = "java-tck-default-region-class-request-" + UUID.randomUUID();
      boolean ownerConnected = false, ordinaryConnected = false, regionalConnected = false;
      boolean created = false, ownerJoined = false, ordinaryJoined = false, regionalJoined = false;
      boolean published = false, sourceRegionCreated = false, broadRegionCreated = false;
      ObjectClassHandle ownerClass = null;
      AttributeHandleSet requested = null;
      AttributeSetRegionSetPairList sourcePair = null;
      ObjectInstanceHandle regionalObject = null;
      RegionHandle sourceRegion = null, broadRegion = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel); ownerConnected = true;
         ordinary.connect(new Recorder().proxy(), callbackModel); ordinaryConnected = true;
         regional.connect(new Recorder().proxy(), callbackModel); regionalConnected = true;
         owner.createFederationExecution(federation, fom); created = true;
         owner.joinFederationExecution("default-region-class-owner", federation);
         ownerJoined = true;
         ordinary.joinFederationExecution("default-region-class-ordinary", federation);
         ordinaryJoined = true;
         regional.joinFederationExecution("default-region-class-regional", federation);
         regionalJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle ordinaryClass = ordinary.getObjectClassHandle(objectClassName);
         ObjectClassHandle regionalClass = regional.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle ordinaryAttribute = ordinary.getAttributeHandle(ordinaryClass, attributeName);
         AttributeHandle regionalAttribute = regional.getAttributeHandle(regionalClass, attributeName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle regionalFirst = regional.getDimensionHandle(firstDimensionName);
         DimensionHandle regionalSecond = regional.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(ordinaryClass)
               && ownerClass.equals(regionalClass)
               && ownerAttribute.equals(ordinaryAttribute)
               && ownerAttribute.equals(regionalAttribute)
               && ownerFirst.equals(regionalFirst) && ownerSecond.equals(regionalSecond)
               && !ownerFirst.equals(ownerSecond),
            "adapter DDM FOM returned inconsistent class, attribute, or dimension handles");
         Set<DimensionHandle> dimensions = owner.getAvailableDimensionsForObjectClass(ownerClass);
         JavaTckSupport.check(dimensions.contains(ownerFirst) && dimensions.contains(ownerSecond)
               && owner.getDimensionUpperBound(ownerFirst) >= 3
               && owner.getDimensionUpperBound(ownerSecond) >= 3,
            "adapter DDM FOM lacks two sufficiently broad object dimensions");

         requested = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet ordinaryAttributes = JavaTckSupport.attributeSet(
            ordinary, ordinaryAttribute);
         AttributeHandleSet regionalAttributes = JavaTckSupport.attributeSet(
            regional, regionalAttribute);
         owner.publishObjectClassAttributes(ownerClass, requested); published = true;
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 0, 1);
         sourceRegionCreated = true;
         broadRegion = createRegion(regional, regionalFirst, regionalSecond, 0, 3);
         broadRegionCreated = true;
         sourcePair = association(owner, requested, sourceRegion);
         AttributeSetRegionSetPairList broadPair =
            association(regional, regionalAttributes, broadRegion);
         regionalObject = owner.registerObjectInstanceWithRegions(ownerClass, sourcePair);
         ObjectInstanceHandle defaultObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(regionalObject != null && defaultObject != null
               && !regionalObject.equals(defaultObject),
            "adapter DDM FOM did not register distinct regional and default-region objects");

         ordinary.requestAttributeValueUpdate(
            ordinaryClass, ordinaryAttributes, ORDINARY_TAG.clone());
         regional.requestAttributeValueUpdateWithRegions(
            regionalClass, broadPair, REGIONAL_TAG.clone());
         long deadline = System.nanoTime() + TIMEOUT_NANOS;
         while (System.nanoTime() < deadline && ownerCallbacks.provided.size() < 4) {
            JavaTckSupport.drain(owner);
            Thread.sleep(1L);
         }
         JavaTckSupport.check(ownerCallbacks.provided.size() == 4,
            "ordinary and regional class requests did not each solicit both objects exactly once");
         Set<ObjectInstanceHandle> ordinaryObjects = new HashSet<>();
         Set<ObjectInstanceHandle> regionalObjects = new HashSet<>();
         for (ProvidedUpdate callback : ownerCallbacks.provided) {
            JavaTckSupport.check(requested.equals(callback.attributes),
               "class request equivalence changed the requested attribute set");
            if (Arrays.equals(ORDINARY_TAG, callback.tag)) ordinaryObjects.add(callback.object);
            else if (Arrays.equals(REGIONAL_TAG, callback.tag)) regionalObjects.add(callback.object);
            else throw new AssertionError("class request callback changed the request tag");
         }
         Set<ObjectInstanceHandle> expected = new HashSet<>(
            Arrays.asList(regionalObject, defaultObject));
         JavaTckSupport.check(ordinaryObjects.equals(expected)
               && regionalObjects.equals(expected),
            "ordinary and broad explicit-region class requests selected different objects");

         owner.unassociateRegionsForUpdates(regionalObject, sourcePair);
         owner.unpublishObjectClassAttributes(ownerClass, requested); published = false;
         regional.deleteRegion(broadRegion); broadRegionCreated = false;
         owner.deleteRegion(sourceRegion); sourceRegionCreated = false;
         regional.resignFederationExecution(ResignAction.NO_ACTION); regionalJoined = false;
         ordinary.resignFederationExecution(ResignAction.NO_ACTION); ordinaryJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation); created = false;
      } finally {
         if (ownerJoined && regionalObject != null && sourcePair != null) try {
            owner.unassociateRegionsForUpdates(regionalObject, sourcePair);
         } catch (Exception ignored) { }
         if (published && ownerJoined) try {
            owner.unpublishObjectClassAttributes(ownerClass, requested);
         } catch (Exception ignored) { }
         if (broadRegionCreated && regionalJoined) try { regional.deleteRegion(broadRegion); }
            catch (Exception ignored) { }
         if (sourceRegionCreated && ownerJoined) try { owner.deleteRegion(sourceRegion); }
            catch (Exception ignored) { }
         if (regionalJoined) try { regional.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ordinaryJoined) try { ordinary.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (regionalConnected) try { regional.disconnect(); } catch (Exception ignored) { }
         if (ordinaryConnected) try { ordinary.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static RegionHandle createRegion(RTIambassador rti, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = rti.getDimensionHandleSetFactory().create();
      dimensions.add(first); dimensions.add(second);
      RegionHandle region = rti.createRegion(dimensions);
      rti.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      rti.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = rti.getRegionHandleSetFactory().create();
      regions.add(region);
      rti.commitRegionModifications(regions);
      return region;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador rti,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      RegionHandleSet regions = rti.getRegionHandleSetFactory().create();
      regions.add(region);
      AttributeSetRegionSetPairList pair = rti.getAttributeSetRegionSetPairListFactory().create(1);
      pair.add(new AttributeRegionAssociation(attributes, regions));
      return pair;
   }

   private static final class ProvidedUpdate {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;
      private ProvidedUpdate(ObjectInstanceHandle object, AttributeHandleSet attributes, byte[] tag) {
         this.object = object; this.attributes = attributes; this.tag = tag;
      }
   }

   private static final class Recorder {
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("provideAttributeValueUpdate".equals(method.getName())
                     && arguments != null && arguments.length == 3) {
                  provided.add(new ProvidedUpdate((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) ((AttributeHandleSet) arguments[1]).clone(),
                     ((byte[]) arguments[2]).clone()));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK class request callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName()))
                     return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
