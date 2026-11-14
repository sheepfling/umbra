package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Isolates replacement of the default update association by one explicit source region. */
final class AssociateRegionsForUpdatesReplacesDefaultRegionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private AssociateRegionsForUpdatesReplacesDefaultRegionTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador overlap = factory.getRtiAmbassador();
      RTIambassador disjoint = factory.getRtiAmbassador();
      Recorder overlapRecorder = new Recorder();
      Recorder disjointRecorder = new Recorder();
      String federation = "java-tck-associate-regions-replaces-default-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean overlapConnected = false;
      boolean disjointConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean overlapJoined = false;
      boolean disjointJoined = false;
      boolean published = false;
      boolean overlapSubscribed = false;
      boolean disjointSubscribed = false;
      boolean associated = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle overlapClass = null;
      ObjectClassHandle disjointClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle overlapAttribute = null;
      AttributeHandle disjointAttribute = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet overlapAttributes = null;
      AttributeHandleSet disjointAttributes = null;
      AttributeSetRegionSetPairList sourcePairs = null;
      AttributeSetRegionSetPairList overlapPairs = null;
      AttributeSetRegionSetPairList disjointPairs = null;
      RegionHandle sourceRegion = null;
      RegionHandle overlapRegion = null;
      RegionHandle disjointRegion = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(new Recorder().proxy(), callbackModel);
         ownerConnected = true;
         overlap.connect(overlapRecorder.proxy(), callbackModel);
         overlapConnected = true;
         disjoint.connect(disjointRecorder.proxy(), callbackModel);
         disjointConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-default-association-owner", federation);
         ownerJoined = true;
         overlap.joinFederationExecution("java-tck-default-association-overlap", federation);
         overlapJoined = true;
         disjoint.joinFederationExecution("java-tck-default-association-disjoint", federation);
         disjointJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         overlapClass = overlap.getObjectClassHandle(objectClassName);
         disjointClass = disjoint.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         overlapAttribute = overlap.getAttributeHandle(overlapClass, attributeName);
         disjointAttribute = disjoint.getAttributeHandle(disjointClass, attributeName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle overlapFirst = overlap.getDimensionHandle(firstDimensionName);
         DimensionHandle overlapSecond = overlap.getDimensionHandle(secondDimensionName);
         DimensionHandle disjointFirst = disjoint.getDimensionHandle(firstDimensionName);
         DimensionHandle disjointSecond = disjoint.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(overlapClass)
               && ownerClass.equals(disjointClass)
               && ownerAttribute.equals(overlapAttribute)
               && ownerAttribute.equals(disjointAttribute)
               && ownerFirst.equals(overlapFirst) && ownerFirst.equals(disjointFirst)
               && ownerSecond.equals(overlapSecond) && ownerSecond.equals(disjointSecond),
            "adapter DDM FOM handles changed across association-replacement federates");
         checkDimensions(owner, ownerClass, ownerFirst, ownerSecond);
         checkDimensions(overlap, overlapClass, overlapFirst, overlapSecond);
         checkDimensions(disjoint, disjointClass, disjointFirst, disjointSecond);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         overlapAttributes = JavaTckSupport.attributeSet(overlap, overlapAttribute);
         disjointAttributes = JavaTckSupport.attributeSet(disjoint, disjointAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;

         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5);
         overlapRegion = createRegion(overlap, overlapFirst, overlapSecond, 2, 4);
         disjointRegion = createRegion(disjoint, disjointFirst, disjointSecond, 7, 9);
         sourcePairs = association(owner, ownerAttributes, sourceRegion);
         overlapPairs = association(overlap, overlapAttributes, overlapRegion);
         disjointPairs = association(disjoint, disjointAttributes, disjointRegion);

         overlap.setConveyRegionDesignatorSetsSwitch(true);
         disjoint.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(overlap.getConveyRegionDesignatorSetsSwitch()
               && disjoint.getConveyRegionDesignatorSetsSwitch(),
            "regional subscribers did not enable source-region designators");
         overlap.subscribeObjectClassAttributesWithRegions(overlapClass, overlapPairs);
         overlapSubscribed = true;
         disjoint.subscribeObjectClassAttributesWithRegions(disjointClass, disjointPairs);
         disjointSubscribed = true;

         object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "default-association registration returned an invalid object handle");
         ObjectInstanceHandle registered = object;
         await(callbackModel,
            () -> overlapRecorder.discoveries.contains(registered)
               && disjointRecorder.discoveries.contains(registered),
            "default-source discovery", overlap, disjoint);

         byte[] defaultValue = new byte[] {(byte) 0xD0, 0x01};
         byte[] defaultTag = new byte[] {(byte) 0xA1, 0x01};
         update(owner, object, ownerAttribute, defaultValue, defaultTag);
         await(callbackModel,
            () -> overlapRecorder.reflections.size() >= 1
               && disjointRecorder.reflections.size() >= 1,
            "default-source reflections", overlap, disjoint);
         JavaTckSupport.check(overlapRecorder.reflections.size() == 1
               && disjointRecorder.reflections.size() == 1,
            "default-source update did not produce one reflection for each regional subscriber");
         assertReflection(overlapRecorder.reflections.get(0), object, overlapAttribute,
            defaultValue, defaultTag, null, "overlapping default-source reflection");
         assertReflection(disjointRecorder.reflections.get(0), object, disjointAttribute,
            defaultValue, defaultTag, null, "disjoint default-source reflection");

         overlapRecorder.reflections.clear();
         disjointRecorder.reflections.clear();
         owner.associateRegionsForUpdates(object, sourcePairs);
         associated = true;
         byte[] explicitValue = new byte[] {(byte) 0xE0, 0x02};
         byte[] explicitTag = new byte[] {(byte) 0xA1, 0x02};
         update(owner, object, ownerAttribute, explicitValue, explicitTag);
         await(callbackModel, () -> !overlapRecorder.reflections.isEmpty(),
            "explicit-source reflection", overlap);
         settle(callbackModel, disjoint);
         JavaTckSupport.check(overlapRecorder.reflections.size() == 1
               && disjointRecorder.reflections.isEmpty(),
            "explicit association did not replace the default source for both subscriptions");
         assertReflection(overlapRecorder.reflections.get(0), object, overlapAttribute,
            explicitValue, explicitTag, sourceRegion, "overlapping explicit-source reflection");
      } finally {
         if (ownerJoined && associated && object != null && sourcePairs != null) {
            try { owner.unassociateRegionsForUpdates(object, sourcePairs); }
            catch (Exception ignored) { }
         }
         if (overlapJoined && overlapSubscribed) {
            try { overlap.unsubscribeObjectClassAttributesWithRegions(overlapClass, overlapPairs); }
            catch (Exception ignored) { }
         }
         if (disjointJoined && disjointSubscribed) {
            try { disjoint.unsubscribeObjectClassAttributesWithRegions(disjointClass, disjointPairs); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         deleteRegion(owner, sourceRegion, ownerJoined);
         deleteRegion(overlap, overlapRegion, overlapJoined);
         deleteRegion(disjoint, disjointRegion, disjointJoined);
         if (overlapJoined) resign(overlap, ResignAction.NO_ACTION);
         if (disjointJoined) resign(disjoint, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (disjointConnected) disconnect(disjoint);
         if (overlapConnected) disconnect(overlap);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      DimensionHandleSet available = ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && !first.equals(second) && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         "adapter object class must expose both configured DDM dimensions through range 9");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new RangeBounds(lower, upper));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
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

   private static void update(RTIambassador owner, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      owner.updateAttributeValues(object, values, tag.clone());
   }

   private static void assertReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag, RegionHandle sourceRegion,
         String description) {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && reflection.values.containsKey(attribute)
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag),
         description + " changed object, attribute value, or tag");
      JavaTckSupport.check(reflection.regions != null
            && (sourceRegion == null ? reflection.regions.isEmpty()
               : reflection.regions.size() == 1),
         description + " returned the wrong source-region designator set");
   }

   private static void await(CallbackModel model, BooleanSupplier condition,
         String description, RTIambassador... ambassadors) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         for (RTIambassador ambassador : ambassadors) service(ambassador, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void settle(CallbackModel model, RTIambassador ambassador) throws Exception {
      long deadline = System.nanoTime() + 100_000_000L;
      while (System.nanoTime() < deadline) {
         service(ambassador, model);
         Thread.sleep(1L);
      }
   }

   private static void service(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(ambassador);
      } else {
         Thread.yield();
      }
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean joined) {
      if (joined && region != null) {
         try { ambassador.deleteRegion(region); } catch (Exception ignored) { }
      }
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final RegionHandleSet regions;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, RegionHandleSet regions) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.regions = regions;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK default-association callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
