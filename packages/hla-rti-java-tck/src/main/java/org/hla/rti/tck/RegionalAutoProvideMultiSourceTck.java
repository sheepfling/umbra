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
import java.util.concurrent.atomic.AtomicReference;

/** Verifies separate regional Auto Provide sources route independently to their subscribers. */
final class RegionalAutoProvideMultiSourceTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] FLAVOR_VALUE = new byte[] {0x4D, 0x53, 0x41};
   private static final byte[] ORGANIC_VALUE = new byte[] {0x4D, 0x53, 0x42};
   private static final byte[] FLAVOR_TAG = new byte[] {0x54, 0x41};
   private static final byte[] ORGANIC_TAG = new byte[] {0x54, 0x42};

   private RegionalAutoProvideMultiSourceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador flavor = factory.getRtiAmbassador();
      RTIambassador organic = factory.getRtiAmbassador();
      AtomicReference<Throwable> callbackFailure = new AtomicReference<>();
      Recorder ownerCallbacks = new Recorder(callbackFailure, true);
      Recorder flavorCallbacks = new Recorder(callbackFailure, false);
      Recorder organicCallbacks = new Recorder(callbackFailure, false);
      String federation = "java-tck-regional-auto-provide-multi-source-" + UUID.randomUUID();
      boolean ownerConnected = false, flavorConnected = false, organicConnected = false;
      boolean created = false, ownerJoined = false, flavorJoined = false, organicJoined = false;
      boolean published = false, flavorSubscribed = false, organicSubscribed = false;
      boolean ownerFlavorRegionCreated = false, ownerOrganicRegionCreated = false;
      boolean flavorRegionCreated = false, organicRegionCreated = false, associated = false;
      ObjectClassHandle ownerClass = null, flavorClass = null, organicClass = null;
      AttributeHandle ownerFlavorAttribute = null, ownerOrganicAttribute = null;
      AttributeHandle flavorAttribute = null, organicAttribute = null;
      AttributeHandleSet ownerAttributes = null, flavorAttributes = null, organicAttributes = null;
      AttributeSetRegionSetPairList ownerPairs = null, flavorPair = null, organicPair = null;
      RegionHandle ownerFlavorRegion = null, ownerOrganicRegion = null;
      RegionHandle flavorRegion = null, organicRegion = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel); ownerConnected = true;
         flavor.connect(flavorCallbacks.proxy(), callbackModel); flavorConnected = true;
         organic.connect(organicCallbacks.proxy(), callbackModel); organicConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "regional-auto-provide-multi-source-owner", "owner", federation); ownerJoined = true;
         flavor.joinFederationExecution("regional-auto-provide-multi-source-flavor", "flavor",
            federation); flavorJoined = true;
         organic.joinFederationExecution("regional-auto-provide-multi-source-organic", "organic",
            federation); organicJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         ownerFlavorAttribute = owner.getAttributeHandle(ownerClass, firstAttributeName);
         ownerOrganicAttribute = owner.getAttributeHandle(ownerClass, secondAttributeName);
         DimensionHandle ownerDimension = owner.getDimensionHandle(dimensionName);
         flavorClass = flavor.getObjectClassHandle(objectClassName);
         flavorAttribute = flavor.getAttributeHandle(flavorClass, firstAttributeName);
         DimensionHandle flavorDimension = flavor.getDimensionHandle(dimensionName);
         organicClass = organic.getObjectClassHandle(objectClassName);
         organicAttribute = organic.getAttributeHandle(organicClass, secondAttributeName);
         DimensionHandle organicDimension = organic.getDimensionHandle(dimensionName);
         JavaTckSupport.check(ownerClass.equals(flavorClass) && ownerClass.equals(organicClass)
               && ownerFlavorAttribute.equals(flavorAttribute)
               && ownerOrganicAttribute.equals(organicAttribute)
               && ownerDimension.equals(flavorDimension) && ownerDimension.equals(organicDimension),
            "adapter Auto Provide FOM returned inconsistent multi-source handles");
         JavaTckSupport.check(owner.getAutoProvideSwitch() && flavor.getAutoProvideSwitch()
               && organic.getAutoProvideSwitch(),
            "adapter FOM did not enable Auto Provide for all multi-source federates");
         checkDimension(owner, ownerClass, ownerDimension);
         checkDimension(flavor, flavorClass, flavorDimension);
         checkDimension(organic, organicClass, organicDimension);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerFlavorAttribute,
            ownerOrganicAttribute);
         flavorAttributes = JavaTckSupport.attributeSet(flavor, flavorAttribute);
         organicAttributes = JavaTckSupport.attributeSet(organic, organicAttribute);
         ownerCallbacks.configureSources(owner, ownerFlavorAttribute, ownerOrganicAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes); published = true;

         ownerFlavorRegion = createRegion(owner, ownerDimension, 0, 2);
         ownerFlavorRegionCreated = true;
         ownerOrganicRegion = createRegion(owner, ownerDimension, 5, 7);
         ownerOrganicRegionCreated = true;
         flavorRegion = createRegion(flavor, flavorDimension, 1, 3); flavorRegionCreated = true;
         organicRegion = createRegion(organic, organicDimension, 6, 8); organicRegionCreated = true;
         ownerPairs = sourceAssociations(owner, ownerFlavorAttribute, ownerFlavorRegion,
            ownerOrganicAttribute, ownerOrganicRegion);
         flavorPair = association(flavor, flavorAttributes, flavorRegion);
         organicPair = association(organic, organicAttributes, organicRegion);
         flavor.setConveyRegionDesignatorSetsSwitch(true);
         organic.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(flavor.getConveyRegionDesignatorSetsSwitch()
               && organic.getConveyRegionDesignatorSetsSwitch(),
            "regional Auto Provide subscribers did not enable region-designator callbacks");

         object = owner.registerObjectInstanceWithRegions(ownerClass, ownerPairs);
         associated = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "multi-source regional Auto Provide registration returned an invalid object handle");
         flavor.subscribeObjectClassAttributesWithRegions(flavorClass, flavorPair);
         flavorSubscribed = true;
         organic.subscribeObjectClassAttributesWithRegions(organicClass, organicPair);
         organicSubscribed = true;

         ObjectInstanceHandle registeredObject = object;
         await(owner, flavor, organic, callbackModel,
            () -> flavorCallbacks.discoveries.size() == 1
               && organicCallbacks.discoveries.size() == 1
               && ownerCallbacks.requests.size() == 2
               && flavorCallbacks.reflections.size() == 1
               && organicCallbacks.reflections.size() == 1,
            "both independent regional Auto Provide deliveries");
         checkCallbackFailure(callbackFailure);

         verifyDiscovery(flavorCallbacks.discoveries.get(0), registeredObject, flavorClass, producer,
            "flavor");
         verifyDiscovery(organicCallbacks.discoveries.get(0), registeredObject, organicClass, producer,
            "organic");
         boolean sawFlavorRequest = false, sawOrganicRequest = false;
         for (Request request : ownerCallbacks.requests) {
            JavaTckSupport.check(registeredObject.equals(request.object) && request.tag.length == 0,
               "multi-source Auto Provide solicitation returned incorrect object or request tag");
            if (request.attributes.size() == 1 && request.attributes.contains(ownerFlavorAttribute)) {
               sawFlavorRequest = true;
            } else if (request.attributes.size() == 1
                  && request.attributes.contains(ownerOrganicAttribute)) {
               sawOrganicRequest = true;
            } else {
               throw new AssertionError("multi-source Auto Provide requested unexpected attributes");
            }
         }
         JavaTckSupport.check(sawFlavorRequest && sawOrganicRequest,
            "Auto Provide did not issue one separate request for each source attribute");

         TransportationTypeHandle reliable = owner.getTransportationTypeHandle("HLAreliable");
         verifyReflection(flavorCallbacks.reflections.get(0), registeredObject, flavorAttribute,
            FLAVOR_VALUE, FLAVOR_TAG, reliable, producer, ownerFlavorRegion, "flavor");
         verifyReflection(organicCallbacks.reflections.get(0), registeredObject, organicAttribute,
            ORGANIC_VALUE, ORGANIC_TAG, reliable, producer, ownerOrganicRegion, "organic");

         flavor.unsubscribeObjectClassAttributesWithRegions(flavorClass, flavorPair);
         flavorSubscribed = false;
         organic.unsubscribeObjectClassAttributesWithRegions(organicClass, organicPair);
         organicSubscribed = false;
         owner.unassociateRegionsForUpdates(registeredObject, ownerPairs); associated = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); published = false;
         deleteRegion(flavor, flavorRegion); flavorRegionCreated = false;
         deleteRegion(organic, organicRegion); organicRegionCreated = false;
         deleteRegion(owner, ownerFlavorRegion); ownerFlavorRegionCreated = false;
         deleteRegion(owner, ownerOrganicRegion); ownerOrganicRegionCreated = false;
         flavor.resignFederationExecution(ResignAction.NO_ACTION); flavorJoined = false;
         organic.resignFederationExecution(ResignAction.NO_ACTION); organicJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation); created = false;
      } finally {
         if (flavorJoined && flavorSubscribed && flavorClass != null && flavorPair != null) try {
            flavor.unsubscribeObjectClassAttributesWithRegions(flavorClass, flavorPair);
         } catch (Exception ignored) { }
         if (organicJoined && organicSubscribed && organicClass != null && organicPair != null) try {
            organic.unsubscribeObjectClassAttributesWithRegions(organicClass, organicPair);
         } catch (Exception ignored) { }
         if (ownerJoined && associated && object != null && ownerPairs != null) try {
            owner.unassociateRegionsForUpdates(object, ownerPairs);
         } catch (Exception ignored) { }
         if (published && ownerJoined && ownerClass != null && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (flavorJoined) deleteRegion(flavor, flavorRegion, flavorRegionCreated);
         if (organicJoined) deleteRegion(organic, organicRegion, organicRegionCreated);
         if (ownerJoined) {
            deleteRegion(owner, ownerFlavorRegion, ownerFlavorRegionCreated);
            deleteRegion(owner, ownerOrganicRegion, ownerOrganicRegionCreated);
         }
         if (flavorJoined) try { flavor.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (organicJoined) try { organic.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (organicConnected) try { organic.disconnect(); } catch (Exception ignored) { }
         if (flavorConnected) try { flavor.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimension(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle dimension) throws Exception {
      java.util.Set<DimensionHandle> available =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(available.size() == 1 && available.contains(dimension)
            && ambassador.getDimensionUpperBound(dimension) >= 8,
         "adapter Auto Provide FOM must expose the configured dimension through range 8");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle dimension,
         long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(dimension);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, dimension,
         new hla.rti1516_2025.RangeBounds(lower, upper));
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

   private static AttributeSetRegionSetPairList sourceAssociations(RTIambassador ambassador,
         AttributeHandle flavorAttribute, RegionHandle flavorRegion,
         AttributeHandle organicAttribute, RegionHandle organicRegion) throws Exception {
      RegionHandleSet flavorRegions = ambassador.getRegionHandleSetFactory().create();
      flavorRegions.add(flavorRegion);
      RegionHandleSet organicRegions = ambassador.getRegionHandleSetFactory().create();
      organicRegions.add(organicRegion);
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(2);
      pairs.add(new AttributeRegionAssociation(
         JavaTckSupport.attributeSet(ambassador, flavorAttribute), flavorRegions));
      pairs.add(new AttributeRegionAssociation(
         JavaTckSupport.attributeSet(ambassador, organicAttribute), organicRegions));
      return pairs;
   }

   private static void await(RTIambassador owner, RTIambassador flavor, RTIambassador organic,
         CallbackModel model, java.util.function.BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(owner, model); pump(flavor, model); pump(organic, model); Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void checkCallbackFailure(AtomicReference<Throwable> failure) {
      if (failure.get() != null) throw new AssertionError("multi-source Auto Provide callback failed",
         failure.get());
   }

   private static void verifyDiscovery(Discovery discovery, ObjectInstanceHandle object,
         ObjectClassHandle objectClass, FederateHandle producer, String subscriber) {
      JavaTckSupport.check(object.equals(discovery.object) && objectClass.equals(discovery.objectClass)
            && producer.equals(discovery.producer),
         subscriber + " subscriber discovery returned incorrect object metadata");
   }

   private static void verifyReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag,
         TransportationTypeHandle reliable, FederateHandle producer, RegionHandle sourceRegion,
         String subscriber) {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag)
            && reliable.equals(reflection.transportation) && producer.equals(reflection.producer)
            && reflection.regions != null && reflection.regions.size() == 1
            && reflection.regions.contains(sourceRegion),
         subscriber + " scoped Auto Provide reflection changed its value or metadata");
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region) {
      deleteRegion(ambassador, region, region != null);
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region, boolean created) {
      if (created && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private FederateHandle producer;
   }

   private static final class Request {
      private ObjectInstanceHandle object;
      private AttributeHandleSet attributes;
      private byte[] tag;
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
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Request> requests = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final AtomicReference<Throwable> callbackFailure;
      private final boolean provider;
      private final AtomicReference<RTIambassador> owner = new AtomicReference<>();
      private final AtomicReference<AttributeHandle> flavorAttribute = new AtomicReference<>();
      private final AtomicReference<AttributeHandle> organicAttribute = new AtomicReference<>();

      private Recorder(AtomicReference<Throwable> callbackFailure, boolean provider) {
         this.callbackFailure = callbackFailure; this.provider = provider;
      }

      private void configureSources(RTIambassador ambassador, AttributeHandle flavor,
            AttributeHandle organic) {
         owner.set(ambassador); flavorAttribute.set(flavor); organicAttribute.set(organic);
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && args != null && args.length >= 3) {
                  Discovery discovery = new Discovery();
                  discovery.object = (ObjectInstanceHandle) args[0];
                  discovery.objectClass = (ObjectClassHandle) args[1];
                  discovery.producer = (FederateHandle) args[2];
                  discoveries.add(discovery);
               } else if ("provideAttributeValueUpdate".equals(name) && provider
                     && args != null && args.length == 3) {
                  Request request = new Request();
                  request.object = (ObjectInstanceHandle) args[0];
                  request.attributes = (AttributeHandleSet) ((AttributeHandleSet) args[1]).clone();
                  request.tag = ((byte[]) args[2]).clone();
                  requests.add(request);
                  try { respond(request); }
                  catch (Throwable failure) { callbackFailure.compareAndSet(null, failure); }
               } else if ("reflectAttributeValues".equals(name) && !provider
                     && args != null && args.length == 6) {
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
                  if ("toString".equals(name)) return "Java TCK regional Auto Provide multi-source callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }

      private void respond(Request request) throws Exception {
         AttributeHandle flavor = flavorAttribute.get();
         AttributeHandle organic = organicAttribute.get();
         if (request.attributes.size() != 1) {
            throw new IllegalStateException("multi-source Auto Provide request did not isolate one attribute");
         }
         AttributeHandle attribute = request.attributes.iterator().next();
         byte[] value;
         byte[] tag;
         if (flavor.equals(attribute)) { value = FLAVOR_VALUE; tag = FLAVOR_TAG; }
         else if (organic.equals(attribute)) { value = ORGANIC_VALUE; tag = ORGANIC_TAG; }
         else throw new IllegalStateException("multi-source Auto Provide requested an unknown attribute");
         AttributeHandleValueMap values = owner.get().getAttributeHandleValueMapFactory().create(1);
         values.put(attribute, value.clone());
         owner.get().updateAttributeValues(request.object, values, tag.clone());
      }
   }
}
