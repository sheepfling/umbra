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
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Rechecks DDM eligibility when an attribute-value request response is delivered. */
final class RegionalAttributeValueUpdateResponseRecheckTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] REQUEST_TAG = new byte[] {0x52, 0x45, 0x51};
   private static final byte[] RESPONSE_VALUE = new byte[] {0x5A, 0x25, 0x07};
   private static final byte[] RESPONSE_TAG = new byte[] {0x52, 0x45, 0x53};

   private RegionalAttributeValueUpdateResponseRecheckTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-regional-value-response-recheck-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false, published = false, subscribed = false;
      boolean ownerRegionCreated = false, requesterRegionCreated = false, associated = false;
      ObjectClassHandle ownerClass = null, requesterClass = null;
      AttributeHandle ownerAttribute = null, requesterAttribute = null;
      AttributeHandleSet ownerAttributes = null, requesterAttributes = null;
      AttributeSetRegionSetPairList ownerPair = null, requesterPair = null;
      RegionHandle ownerRegion = null, requesterRegion = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producingFederate = owner.joinFederationExecution(
            "java-tck-regional-response-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-regional-response-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle requesterFirst = requester.getDimensionHandle(firstDimensionName);
         DimensionHandle requesterSecond = requester.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute)
               && ownerFirst.equals(requesterFirst) && ownerSecond.equals(requesterSecond),
            "adapter DDM FOM returned inconsistent object, attribute, or dimension handles");
         checkDimensions(owner, ownerClass, ownerFirst, ownerSecond);
         checkDimensions(requester, requesterClass, requesterFirst, requesterSecond);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         ownerRegion = createRegion(owner, ownerFirst, ownerSecond, 0, 1);
         ownerRegionCreated = true;
         requesterRegion = createRegion(requester, requesterFirst, requesterSecond, 0, 1);
         requesterRegionCreated = true;
         ownerPair = association(owner, ownerAttributes, ownerRegion);
         requesterPair = association(requester, requesterAttributes, requesterRegion);
         requester.setConveyRegionDesignatorSetsSwitch(true);
         requester.subscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         subscribed = true;
         object = owner.registerObjectInstanceWithRegions(ownerClass, ownerPair);
         associated = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional attribute-value response registration returned an invalid object handle");
         ObjectInstanceHandle registeredObject = object;
         await(owner, requester, callbackModel,
            () -> requesterCallbacks.discoveries.contains(registeredObject),
            "regional response object discovery");

         request(owner, requester, requesterClass, requesterPair, callbackModel,
            ownerCallbacks, "initial regional value request");
         JavaTckSupport.check(ownerCallbacks.provided.size() == 1,
            "initial regional value request returned an unexpected provider callback count");
         assertProvidedUpdate(ownerCallbacks.provided.get(0), object, ownerAttributes,
            REQUEST_TAG, "initial regional provider callback");

         setBothAndCommit(requester, requesterRegion, requesterFirst, requesterSecond, 2, 3);
         respond(owner, object, ownerAttribute);
         pumpSeveral(requester, callbackModel);
         JavaTckSupport.check(requesterCallbacks.reflections.isEmpty(),
            "regional response reflected after the requester moved out of overlap");

         setBothAndCommit(requester, requesterRegion, requesterFirst, requesterSecond, 0, 1);
         ownerCallbacks.provided.clear();
         requesterCallbacks.reflections.clear();
         request(owner, requester, requesterClass, requesterPair, callbackModel,
            ownerCallbacks, "restored-overlap regional value request");
         JavaTckSupport.check(ownerCallbacks.provided.size() == 1,
            "restored regional request returned an unexpected provider callback count");
         assertProvidedUpdate(ownerCallbacks.provided.get(0), object, ownerAttributes,
            REQUEST_TAG, "restored-overlap provider callback");
         respond(owner, object, ownerAttribute);
         await(owner, requester, callbackModel,
            () -> requesterCallbacks.reflections.size() == 1,
            "restored-overlap attribute reflection");
         assertReflection(requester, requesterCallbacks.reflections.get(0), object,
            requesterAttribute, producingFederate, ownerRegion, RESPONSE_VALUE, RESPONSE_TAG);

         requester.unsubscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         subscribed = false;
         owner.unassociateRegionsForUpdates(object, ownerPair);
         associated = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         published = false;
         requester.deleteRegion(requesterRegion);
         requesterRegionCreated = false;
         owner.deleteRegion(ownerRegion);
         ownerRegionCreated = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && requesterJoined && requesterPair != null) try {
            requester.unsubscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         } catch (Exception ignored) { }
         if (associated && ownerJoined && object != null && ownerPair != null) try {
            owner.unassociateRegionsForUpdates(object, ownerPair);
         } catch (Exception ignored) { }
         if (published && ownerJoined) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         deleteRegion(requester, requesterRegion, requesterRegionCreated, requesterJoined);
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

   private static void request(RTIambassador owner, RTIambassador requester,
         ObjectClassHandle objectClass, AttributeSetRegionSetPairList pair, CallbackModel model,
         Recorder ownerCallbacks, String description) throws Exception {
      ownerCallbacks.provided.clear();
      requester.requestAttributeValueUpdateWithRegions(objectClass, pair, REQUEST_TAG.clone());
      awaitOwner(owner, model, () -> ownerCallbacks.provided.size() >= 1, description);
   }

   private static void respond(RTIambassador owner, ObjectInstanceHandle object,
         AttributeHandle attribute) throws Exception {
      AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, RESPONSE_VALUE.clone());
      owner.updateAttributeValues(object, values, RESPONSE_TAG.clone());
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      java.util.Set<DimensionHandle> dimensions =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(dimensions.contains(first) && dimensions.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 3
            && ambassador.getDimensionUpperBound(second) >= 3,
         "adapter FOM object class does not expose the required DDM dimensions and bounds");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      setBothAndCommit(ambassador, region, first, second, lower, upper);
      return region;
   }

   private static void setBothAndCommit(RTIambassador ambassador, RegionHandle region,
         DimensionHandle first, DimensionHandle second, long lower, long upper) throws Exception {
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
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

   private static void assertProvidedUpdate(ProvidedUpdate update, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(update.object) && attributes.equals(update.attributes)
            && Arrays.equals(tag, update.tag),
         description + " returned the wrong object, attribute set, or request tag");
   }

   private static void assertReflection(RTIambassador requester, Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle attribute, FederateHandle producer,
         RegionHandle sourceRegion, byte[] value, byte[] tag) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer),
         "restored regional response returned the wrong object, value, tag, or producer");
      TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
      JavaTckSupport.check(reliable.equals(reflection.transportation),
         "restored regional response returned a non-reliable transportation type");
      JavaTckSupport.check(reflection.regions != null && reflection.regions.size() == 1
            && reflection.regions.contains(sourceRegion),
         "restored regional response omitted its source-region designator");
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

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         pump(second, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpSeveral(RTIambassador ambassador, CallbackModel model) throws Exception {
      for (int pass = 0; pass < 8; ++pass) pump(ambassador, model);
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

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private RegionHandleSet regions;
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("provideAttributeValueUpdate".equals(name) && arguments != null
                     && arguments.length == 3) {
                  provided.add(new ProvidedUpdate((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) ((AttributeHandleSet) arguments[1]).clone(),
                     ((byte[]) arguments[2]).clone()));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  reflection.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflection.tag = ((byte[]) arguments[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) arguments[3];
                  reflection.producer = (FederateHandle) arguments[4];
                  reflection.regions = (RegionHandleSet) arguments[5];
                  reflections.add(reflection);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional value-response callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
