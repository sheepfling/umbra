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
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Integrated standard-API regional registration, discovery, update, and reassociation case. */
final class RegionalObjectUpdateTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] FIRST_VALUE = new byte[] {0x11, 0x22, 0x33};
   private static final byte[] FIRST_TAG = new byte[] {0x52, 0x31};
   private static final byte[] REQUEST_TAG = new byte[] {0x52, 0x45, 0x51};
   private static final byte[] RESPONSE_VALUE = new byte[] {0x5a, 0x25, 0x07};
   private static final byte[] RESPONSE_TAG = new byte[] {0x52, 0x45, 0x53};

   private RegionalObjectUpdateTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, objectClassName, attributeName, firstDimensionName,
         secondDimensionName, null, callbackModel);
   }

   static void runCustomTransportation(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String firstDimensionName, String secondDimensionName,
         String transportationName, CallbackModel callbackModel) throws Exception {
      run(factory, fom, objectClassName, attributeName, firstDimensionName,
         secondDimensionName, transportationName, callbackModel);
   }

   private static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String firstDimensionName, String secondDimensionName,
         String expectedTransportationName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador overlap = factory.getRtiAmbassador();
      RTIambassador disjoint = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder overlapCallbacks = new Recorder();
      Recorder disjointCallbacks = new Recorder();
      String federation = "java-tck-regional-object-update-" + UUID.randomUUID();
      boolean ownerConnected = false, overlapConnected = false, disjointConnected = false;
      boolean created = false, ownerJoined = false, overlapJoined = false, disjointJoined = false;
      boolean published = false, overlapSubscribed = false, disjointSubscribed = false;
      boolean sourceCreated = false, reassociatedCreated = false;
      boolean overlapRegionCreated = false, disjointRegionCreated = false;
      ObjectClassHandle ownerClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeSetRegionSetPairList sourcePair = null, reassociatedPair = null;
      AttributeSetRegionSetPairList overlapPair = null, disjointPair = null;
      RegionHandle sourceRegion = null, reassociatedRegion = null;
      RegionHandle overlapRegion = null, disjointRegion = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         overlap.connect(overlapCallbacks.proxy(), callbackModel);
         overlapConnected = true;
         disjoint.connect(disjointCallbacks.proxy(), callbackModel);
         disjointConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = owner.joinFederationExecution("regional-update-owner", federation);
         ownerJoined = true;
         overlap.joinFederationExecution("regional-update-overlap", federation);
         overlapJoined = true;
         disjoint.joinFederationExecution("regional-update-disjoint", federation);
         disjointJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle overlapClass = overlap.getObjectClassHandle(objectClassName);
         ObjectClassHandle disjointClass = disjoint.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle overlapAttribute = overlap.getAttributeHandle(overlapClass, attributeName);
         AttributeHandle disjointAttribute = disjoint.getAttributeHandle(disjointClass, attributeName);
         TransportationTypeHandle expectedOwnerTransportation = expectedTransportationName == null
            ? null : owner.getTransportationTypeHandle(expectedTransportationName);
         TransportationTypeHandle expectedOverlapTransportation = expectedTransportationName == null
            ? null : overlap.getTransportationTypeHandle(expectedTransportationName);
         TransportationTypeHandle expectedDisjointTransportation = expectedTransportationName == null
            ? null : disjoint.getTransportationTypeHandle(expectedTransportationName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle overlapFirst = overlap.getDimensionHandle(firstDimensionName);
         DimensionHandle overlapSecond = overlap.getDimensionHandle(secondDimensionName);
         DimensionHandle disjointFirst = disjoint.getDimensionHandle(firstDimensionName);
         DimensionHandle disjointSecond = disjoint.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(overlapClass) && ownerClass.equals(disjointClass)
               && ownerAttribute.equals(overlapAttribute) && ownerAttribute.equals(disjointAttribute)
               && ownerFirst.equals(overlapFirst) && ownerFirst.equals(disjointFirst)
               && ownerSecond.equals(overlapSecond) && ownerSecond.equals(disjointSecond),
            "adapter DDM FOM returned inconsistent regional object handles");
         if (expectedTransportationName != null) {
            JavaTckSupport.check(expectedTransportationName.equals(
                  owner.getTransportationTypeName(expectedOwnerTransportation))
                  && expectedTransportationName.equals(
                     overlap.getTransportationTypeName(expectedOverlapTransportation))
                  && expectedTransportationName.equals(
                     disjoint.getTransportationTypeName(expectedDisjointTransportation)),
               "adapter FOM custom transportation name did not round-trip");
         }
         checkDimensions(owner, ownerClass, ownerFirst, ownerSecond);
         checkDimensions(overlap, overlapClass, overlapFirst, overlapSecond);
         checkDimensions(disjoint, disjointClass, disjointFirst, disjointSecond);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet overlapAttributes = JavaTckSupport.attributeSet(overlap, overlapAttribute);
         AttributeHandleSet disjointAttributes = JavaTckSupport.attributeSet(disjoint, disjointAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5);
         sourceCreated = true;
         reassociatedRegion = createRegion(owner, ownerFirst, ownerSecond, 7, 9);
         reassociatedCreated = true;
         overlapRegion = createRegion(overlap, overlapFirst, overlapSecond, 2, 6);
         overlapRegionCreated = true;
         disjointRegion = createRegion(disjoint, disjointFirst, disjointSecond, 7, 9);
         disjointRegionCreated = true;
         sourcePair = association(owner, ownerAttributes, sourceRegion);
         reassociatedPair = association(owner, ownerAttributes, reassociatedRegion);
         overlapPair = association(overlap, overlapAttributes, overlapRegion);
         disjointPair = association(disjoint, disjointAttributes, disjointRegion);

         overlap.setConveyRegionDesignatorSetsSwitch(true);
         disjoint.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(overlap.getConveyRegionDesignatorSetsSwitch()
               && disjoint.getConveyRegionDesignatorSetsSwitch(),
            "receivers did not enable region-designator callbacks");
         overlap.subscribeObjectClassAttributesWithRegions(overlapClass, overlapPair);
         overlapSubscribed = true;
         disjoint.subscribeObjectClassAttributesWithRegions(disjointClass, disjointPair);
         disjointSubscribed = true;

         String objectName = federation + "-named-regional-object";
         owner.reserveObjectInstanceName(objectName);
         await(owner, callbackModel,
            () -> ownerCallbacks.reservations.contains(objectName), "named object reservation");
         object = owner.registerObjectInstanceWithRegions(ownerClass, sourcePair, objectName);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "named regional registration returned an invalid handle");
         ObjectInstanceHandle registered = object;
         await(overlap, callbackModel, () -> overlapCallbacks.discoveries.contains(registered),
            "overlap-only discovery");
         pumpSeveral(disjoint, callbackModel);
         JavaTckSupport.check(!disjointCallbacks.discoveries.contains(object),
            "disjoint subscription discovered the named regional object");

         AttributeHandleValueMap firstValues = owner.getAttributeHandleValueMapFactory().create(1);
         firstValues.put(ownerAttribute, FIRST_VALUE.clone());
         owner.updateAttributeValues(object, firstValues, FIRST_TAG.clone());
         await(overlap, callbackModel, () -> !overlapCallbacks.reflections.isEmpty(),
            "overlapping regional reflection");
         Reflection first = overlapCallbacks.reflections.get(0);
         assertReflection(first, object, overlapAttribute, producer, FIRST_VALUE, FIRST_TAG,
            sourceRegion, expectedTransportationName, overlap, "initial regional reflection");
         pumpSeveral(disjoint, callbackModel);
         JavaTckSupport.check(disjointCallbacks.reflections.isEmpty(),
            "disjoint subscription received the initial update");

         overlap.requestAttributeValueUpdateWithRegions(overlapClass,
            association(overlap, overlapAttributes, overlapRegion), REQUEST_TAG.clone());
         await(owner, callbackModel, () -> !ownerCallbacks.provided.isEmpty(),
            "regional provide-attribute-value callback");
         ProvidedUpdate provided = ownerCallbacks.provided.get(0);
         JavaTckSupport.check(object.equals(provided.object)
               && ownerAttributes.equals(provided.attributes)
               && Arrays.equals(REQUEST_TAG, provided.tag),
            "regional value request changed object, attribute, or tag");
         AttributeHandleValueMap response = owner.getAttributeHandleValueMapFactory().create(1);
         response.put(ownerAttribute, RESPONSE_VALUE.clone());
         owner.updateAttributeValues(object, response, RESPONSE_TAG.clone());
         await(overlap, callbackModel, () -> overlapCallbacks.reflections.size() >= 2,
            "requested regional value reflection");
         assertReflection(overlapCallbacks.reflections.get(1), object, overlapAttribute,
            producer, RESPONSE_VALUE, RESPONSE_TAG, sourceRegion, expectedTransportationName,
            overlap, "requested regional reflection");

         owner.associateRegionsForUpdates(object, reassociatedPair);
         owner.unassociateRegionsForUpdates(object, sourcePair);
         await(disjoint, callbackModel, () -> disjointCallbacks.discoveries.contains(registered),
            "disjoint discovery after source reassociation");
         overlapCallbacks.reflections.clear();
         disjointCallbacks.reflections.clear();
         AttributeHandleValueMap movedValues = owner.getAttributeHandleValueMapFactory().create(1);
         byte[] movedValue = new byte[] {0x77, (byte) 0x88};
         movedValues.put(ownerAttribute, movedValue);
         owner.updateAttributeValues(object, movedValues, RESPONSE_TAG.clone());
         await(disjoint, callbackModel, () -> !disjointCallbacks.reflections.isEmpty(),
            "reassociated regional reflection");
         assertReflection(disjointCallbacks.reflections.get(0), object, disjointAttribute,
            producer, movedValue, RESPONSE_TAG, reassociatedRegion, expectedTransportationName,
            disjoint, "reassociated reflection");
         pumpSeveral(overlap, callbackModel);
         JavaTckSupport.check(overlapCallbacks.reflections.isEmpty(),
            "old overlapping subscription received an update after reassociation");

         owner.unassociateRegionsForUpdates(object, reassociatedPair);
         sourcePair = null;
         reassociatedPair = null;
         overlap.unsubscribeObjectClassAttributesWithRegions(overlapClass, overlapPair);
         overlapSubscribed = false;
         disjoint.unsubscribeObjectClassAttributesWithRegions(disjointClass, disjointPair);
         disjointSubscribed = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         published = false;
         deleteRegion(owner, reassociatedRegion); reassociatedCreated = false;
         deleteRegion(owner, sourceRegion); sourceCreated = false;
         deleteRegion(overlap, overlapRegion); overlapRegionCreated = false;
         deleteRegion(disjoint, disjointRegion); disjointRegionCreated = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         overlap.resignFederationExecution(ResignAction.NO_ACTION); overlapJoined = false;
         disjoint.resignFederationExecution(ResignAction.NO_ACTION); disjointJoined = false;
         owner.destroyFederationExecution(federation); created = false;
      } finally {
         if (ownerJoined && object != null && reassociatedPair != null) try {
            owner.unassociateRegionsForUpdates(object, reassociatedPair);
         } catch (Exception ignored) { }
         if (ownerJoined && object != null && sourcePair != null) try {
            owner.unassociateRegionsForUpdates(object, sourcePair);
         } catch (Exception ignored) { }
         if (overlapJoined && overlapSubscribed && overlapPair != null) try {
            overlap.unsubscribeObjectClassAttributesWithRegions(
               overlap.getObjectClassHandle(objectClassName), overlapPair);
         } catch (Exception ignored) { }
         if (disjointJoined && disjointSubscribed && disjointPair != null) try {
            disjoint.unsubscribeObjectClassAttributesWithRegions(
               disjoint.getObjectClassHandle(objectClassName), disjointPair);
         } catch (Exception ignored) { }
         if (published && ownerJoined && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (ownerJoined) {
            deleteRegion(owner, reassociatedRegion, reassociatedCreated);
            deleteRegion(owner, sourceRegion, sourceCreated);
         }
         if (overlapJoined) deleteRegion(overlap, overlapRegion, overlapRegionCreated);
         if (disjointJoined) deleteRegion(disjoint, disjointRegion, disjointRegionCreated);
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (overlapJoined) try { overlap.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (disjointJoined) try { disjoint.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
         if (overlapConnected) try { overlap.disconnect(); } catch (Exception ignored) { }
         if (disjointConnected) try { disjoint.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      java.util.Set<DimensionHandle> dimensions =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(dimensions.contains(first) && dimensions.contains(second)
            && !first.equals(second) && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         "adapter FOM must expose two distinct object dimensions with bounds through 9");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first); dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
      return region;
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

   private static void assertReflection(Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle attribute, FederateHandle producer,
         byte[] value, byte[] tag, RegionHandle sourceRegion, String expectedTransportationName,
         RTIambassador receiver, String description) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer),
         description + " changed object, value, tag, or producer");
      JavaTckSupport.check(reflection.transportation != null,
         description + " returned an invalid transportation handle");
      if (expectedTransportationName != null) {
         JavaTckSupport.check(expectedTransportationName.equals(
               receiver.getTransportationTypeName(reflection.transportation)),
            description + " changed the adapter-declared custom transportation type");
      }
      JavaTckSupport.check(reflection.regions != null && reflection.regions.size() == 1
            && reflection.regions.contains(sourceRegion),
         description + " omitted the source-region designator");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpSeveral(RTIambassador ambassador, CallbackModel model) throws Exception {
      for (int pass = 0; pass < 8; ++pass) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region) {
      deleteRegion(ambassador, region, region != null);
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region, boolean created) {
      if (created && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static final class ProvidedUpdate {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;
      private ProvidedUpdate(ObjectInstanceHandle object, AttributeHandleSet attributes, byte[] tag) {
         this.object = object; this.attributes = attributes; this.tag = tag;
      }
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private RegionHandleSet regions;
   }

   private static final class Recorder {
      private final List<String> reservations = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("objectInstanceNameReservationSucceeded".equals(name) && args != null) {
                  reservations.add((String) args[0]);
               } else if ("discoverObjectInstance".equals(name) && args != null) {
                  discoveries.add((ObjectInstanceHandle) args[0]);
               } else if ("provideAttributeValueUpdate".equals(name) && args != null
                     && args.length == 3) {
                  provided.add(new ProvidedUpdate((ObjectInstanceHandle) args[0],
                     (AttributeHandleSet) ((AttributeHandleSet) args[1]).clone(),
                     ((byte[]) args[2]).clone()));
               } else if ("reflectAttributeValues".equals(name) && args != null
                     && args.length == 6) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) args[0];
                  reflection.values = ((AttributeHandleValueMap) args[1]).clone();
                  reflection.tag = ((byte[]) args[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) args[3];
                  reflection.producer = (FederateHandle) args[4];
                  reflection.regions = (RegionHandleSet) args[5];
                  reflections.add(reflection);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional object-update callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
