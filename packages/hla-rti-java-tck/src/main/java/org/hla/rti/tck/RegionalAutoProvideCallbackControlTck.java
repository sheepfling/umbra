package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
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
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies callback disable/enable gates regional discovery and Auto Provide delivery. */
final class RegionalAutoProvideCallbackControlTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private RegionalAutoProvideCallbackControlTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String dimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      List<String> callbackOrder = new CopyOnWriteArrayList<>();
      Recorder ownerCallbacks = new Recorder(callbackOrder, false);
      Recorder requesterCallbacks = new Recorder(callbackOrder, true);
      String federation = "java-tck-regional-auto-provide-callback-control-" + UUID.randomUUID();
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
         owner.connect(ownerCallbacks.proxy(), callbackModel); ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel); requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "regional-auto-provide-callback-control-owner", "owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("regional-auto-provide-callback-control-requester",
            "requester", federation); requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         DimensionHandle ownerDimension = owner.getDimensionHandle(dimensionName);
         DimensionHandle requesterDimension = requester.getDimensionHandle(dimensionName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute)
               && ownerDimension.equals(requesterDimension),
            "adapter Auto Provide FOM returned inconsistent callback-control handles");
         JavaTckSupport.check(owner.getAutoProvideSwitch() && requester.getAutoProvideSwitch(),
            "adapter FOM did not enable the standard Auto Provide switch");
         checkDimension(owner, ownerClass, ownerDimension);
         checkDimension(requester, requesterClass, requesterDimension);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes); published = true;
         ownerRegion = createRegion(owner, ownerDimension, 0, 2); ownerRegionCreated = true;
         requesterRegion = createRegion(requester, requesterDimension, 1, 3);
         requesterRegionCreated = true;
         ownerPair = association(owner, ownerAttributes, ownerRegion);
         requesterPair = association(requester, requesterAttributes, requesterRegion);
         requester.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(requester.getConveyRegionDesignatorSetsSwitch(),
            "callback-control requester did not enable region-designator callbacks");

         requester.disableCallbacks();
         object = owner.registerObjectInstanceWithRegions(ownerClass, ownerPair);
         associated = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "callback-control regional registration returned an invalid object handle");
         requester.subscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         subscribed = true;
         for (int i = 0; i < 4; ++i) {
            pump(requester, callbackModel); pump(owner, callbackModel);
         }
         JavaTckSupport.check(requesterCallbacks.discoveries.isEmpty()
               && ownerCallbacks.provided.isEmpty()
               && requesterCallbacks.reflections.isEmpty() && callbackOrder.isEmpty(),
            "regional discovery or Auto Provide callbacks escaped while requester callbacks were disabled");

         requester.enableCallbacks();
         await(owner, requester, callbackModel,
            () -> requesterCallbacks.discoveries.size() == 1 && ownerCallbacks.provided.size() == 1,
            "callback re-enable to release discovery and Auto Provide callbacks");
         JavaTckSupport.check(requesterCallbacks.reflections.isEmpty(),
            "regional Auto Provide callback-control unexpectedly reflected without an update");
         JavaTckSupport.check(callbackOrder.equals(Arrays.asList("discover", "provide")),
            "regional Auto Provide callbacks did not resume in discovery-then-provider order");
         Discovery discovery = requesterCallbacks.discoveries.get(0);
         JavaTckSupport.check(object.equals(discovery.object)
               && requesterClass.equals(discovery.objectClass)
               && producer.equals(discovery.producer),
            "callback-control discovery returned incorrect object metadata");
         ProvidedUpdate provided = ownerCallbacks.provided.get(0);
         JavaTckSupport.check(object.equals(provided.object)
               && provided.attributes.size() == 1
               && provided.attributes.contains(ownerAttribute) && provided.tag.length == 0,
            "callback-control Auto Provide request returned incorrect object, attributes, or tag");

         requester.unsubscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         subscribed = false;
         owner.unassociateRegionsForUpdates(object, ownerPair); associated = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); published = false;
         deleteRegion(requester, requesterRegion, requesterRegionCreated);
         requesterRegionCreated = false;
         deleteRegion(owner, ownerRegion, ownerRegionCreated); ownerRegionCreated = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION); requesterJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation); created = false;
      } finally {
         if (requesterJoined && subscribed && requesterClass != null && requesterPair != null) try {
            requester.unsubscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         } catch (Exception ignored) { }
         if (ownerJoined && associated && object != null && ownerPair != null) try {
            owner.unassociateRegionsForUpdates(object, ownerPair);
         } catch (Exception ignored) { }
         if (published && ownerJoined && ownerClass != null && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (requesterJoined) deleteRegion(requester, requesterRegion, requesterRegionCreated);
         if (ownerJoined) deleteRegion(owner, ownerRegion, ownerRegionCreated);
         if (requesterJoined) try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimension(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle dimension) throws Exception {
      java.util.Set<DimensionHandle> available =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(available.size() == 1 && available.contains(dimension)
            && ambassador.getDimensionUpperBound(dimension) >= 3,
         "adapter Auto Provide object class must expose its configured dimension through range 3");
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

   private static void await(RTIambassador owner, RTIambassador requester, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(requester, model); pump(owner, model); Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean created) {
      if (created && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private FederateHandle producer;
   }

   private static final class ProvidedUpdate {
      private ObjectInstanceHandle object;
      private AttributeHandleSet attributes;
      private byte[] tag;
   }

   private static final class Recorder {
      private final List<String> callbackOrder;
      private final boolean requester;
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();
      private final List<Object> reflections = new CopyOnWriteArrayList<>();

      private Recorder(List<String> callbackOrder, boolean requester) {
         this.callbackOrder = callbackOrder; this.requester = requester;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && requester && args != null
                     && args.length >= 3) {
                  Discovery discovery = new Discovery();
                  discovery.object = (ObjectInstanceHandle) args[0];
                  discovery.objectClass = (ObjectClassHandle) args[1];
                  discovery.producer = (FederateHandle) args[2];
                  discoveries.add(discovery); callbackOrder.add("discover");
               } else if ("provideAttributeValueUpdate".equals(name) && !requester
                     && args != null && args.length == 3) {
                  ProvidedUpdate update = new ProvidedUpdate();
                  update.object = (ObjectInstanceHandle) args[0];
                  update.attributes = (AttributeHandleSet) ((AttributeHandleSet) args[1]).clone();
                  update.tag = ((byte[]) args[2]).clone();
                  provided.add(update); callbackOrder.add("provide");
               } else if ("reflectAttributeValues".equals(name) && requester
                     && args != null && args.length >= 3) {
                  reflections.add(args[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional callback-control callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
