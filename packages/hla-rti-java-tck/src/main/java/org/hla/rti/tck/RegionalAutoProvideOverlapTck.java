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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/** Verifies regional Auto Provide is suppressed when discovery moves a subscription out of scope. */
final class RegionalAutoProvideOverlapTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private RegionalAutoProvideOverlapTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation, String objectClassName,
         String attributeName, String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      AtomicReference<Throwable> callbackFailure = new AtomicReference<>();
      AtomicBoolean moveSubscriptionOutOfScope = new AtomicBoolean();
      Recorder ownerCallbacks = new Recorder(callbackFailure);
      Recorder requesterCallbacks = new Recorder(callbackFailure);
      String federation = "java-tck-regional-auto-provide-overlap-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false, published = false, subscribed = false;
      boolean ownerRegionCreated = false, requesterRegionCreated = false;
      boolean firstAssociated = false, secondAssociated = false;
      ObjectClassHandle ownerClass = null, requesterClass = null;
      AttributeHandle ownerAttribute = null, requesterAttribute = null;
      AttributeHandleSet ownerAttributes = null, requesterAttributes = null;
      AttributeSetRegionSetPairList ownerPair = null, requesterPair = null;
      RegionHandle ownerRegion = null, requesterRegion = null;
      ObjectInstanceHandle firstObject = null, secondObject = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel); ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel); requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "regional-auto-provide-overlap-owner", "owner", federation); ownerJoined = true;
         requester.joinFederationExecution("regional-auto-provide-overlap-requester", "requester",
            federation); requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         DimensionHandle ownerDimension = owner.getDimensionHandle(dimensionName);
         DimensionHandle requesterDimension = requester.getDimensionHandle(dimensionName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute)
               && ownerDimension.equals(requesterDimension),
            "adapter Auto Provide FOM returned inconsistent regional handles");
         JavaTckSupport.check(owner.getAutoProvideSwitch() && requester.getAutoProvideSwitch(),
            "adapter FOM did not enable the standard Auto Provide switch");
         checkDimension(owner, ownerClass, ownerDimension);
         checkDimension(requester, requesterClass, requesterDimension);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         ownerCallbacks.configureExpectedAttribute(ownerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes); published = true;
         ownerRegion = createRegion(owner, ownerDimension, 0, 2); ownerRegionCreated = true;
         requesterRegion = createRegion(requester, requesterDimension, 1, 3);
         requesterRegionCreated = true;
         ownerPair = association(owner, ownerAttributes, ownerRegion);
         requesterPair = association(requester, requesterAttributes, requesterRegion);
         requesterCallbacks.configureDiscoveryScopeChange(requester, requesterRegion,
            requesterDimension, moveSubscriptionOutOfScope);
         requester.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(requester.getConveyRegionDesignatorSetsSwitch(),
            "regional Auto Provide requester did not enable region-designator callbacks");

         firstObject = owner.registerObjectInstanceWithRegions(ownerClass, ownerPair);
         firstAssociated = true;
         JavaTckSupport.check(firstObject != null && firstObject.encodedLength() > 0,
            "first regional Auto Provide registration returned an invalid object handle");
         requester.subscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         subscribed = true;
         await(owner, requester, callbackModel,
            () -> requesterCallbacks.discoveries.size() == 1 && ownerCallbacks.requests.size() == 1,
            "initial in-scope discovery and Auto Provide request");
         checkCallbackFailure(callbackFailure);
         Discovery firstDiscovery = requesterCallbacks.discoveries.get(0);
         Request firstRequest = ownerCallbacks.requests.get(0);
         JavaTckSupport.check(firstObject.equals(firstDiscovery.object)
               && requesterClass.equals(firstDiscovery.objectClass)
               && producer.equals(firstDiscovery.producer),
            "initial regional Auto Provide discovery returned incorrect metadata");
         JavaTckSupport.check(firstObject.equals(firstRequest.object)
               && firstRequest.attributes.size() == 1
               && firstRequest.attributes.contains(ownerAttribute)
               && firstRequest.tag.length == 0,
            "initial in-scope Auto Provide request returned incorrect metadata");

         moveSubscriptionOutOfScope.set(true);
         secondObject = owner.registerObjectInstanceWithRegions(ownerClass, ownerPair);
         secondAssociated = true;
         JavaTckSupport.check(secondObject != null && secondObject.encodedLength() > 0,
            "second regional Auto Provide registration returned an invalid object handle");
         await(owner, requester, callbackModel,
            () -> requesterCallbacks.discoveries.size() == 2,
            "second discovery after moving the subscription out of scope");
         checkCallbackFailure(callbackFailure);
         Discovery secondDiscovery = requesterCallbacks.discoveries.get(1);
         JavaTckSupport.check(secondObject.equals(secondDiscovery.object)
               && requesterClass.equals(secondDiscovery.objectClass)
               && producer.equals(secondDiscovery.producer),
            "second regional Auto Provide discovery returned incorrect metadata");
         JavaTckSupport.check(ownerCallbacks.requests.size() == 1,
            "stale Auto Provide solicitation was issued after discovery moved the subscription out of scope");
         JavaTckSupport.check(requesterCallbacks.discoveries.size() == 2
               && requesterCallbacks.discoveries.get(0).object.equals(firstObject)
               && requesterCallbacks.discoveries.get(1).object.equals(secondObject),
            "regional Auto Provide delivered an unexpected discovery count or order");

         requester.unsubscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         subscribed = false;
         owner.unassociateRegionsForUpdates(firstObject, ownerPair); firstAssociated = false;
         owner.unassociateRegionsForUpdates(secondObject, ownerPair); secondAssociated = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); published = false;
         deleteRegion(requester, requesterRegion); requesterRegionCreated = false;
         deleteRegion(owner, ownerRegion); ownerRegionCreated = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION); requesterJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation); created = false;
      } finally {
         if (requesterJoined && subscribed && requesterClass != null && requesterPair != null) try {
            requester.unsubscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         } catch (Exception ignored) { }
         if (ownerJoined && firstAssociated && firstObject != null && ownerPair != null) try {
            owner.unassociateRegionsForUpdates(firstObject, ownerPair);
         } catch (Exception ignored) { }
         if (ownerJoined && secondAssociated && secondObject != null && ownerPair != null) try {
            owner.unassociateRegionsForUpdates(secondObject, ownerPair);
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
            && ambassador.getDimensionUpperBound(dimension) >= 5,
         "adapter Auto Provide object class must expose its configured dimension through range 5");
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
         pump(owner, model); pump(requester, model); Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void checkCallbackFailure(AtomicReference<Throwable> failure) {
      if (failure.get() != null) throw new AssertionError("regional Auto Provide callback failed",
         failure.get());
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

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Request> requests = new CopyOnWriteArrayList<>();
      private final AtomicReference<AttributeHandle> expectedAttribute = new AtomicReference<>();
      private final AtomicReference<RTIambassador> scopeOwner = new AtomicReference<>();
      private final AtomicReference<RegionHandle> subscriptionRegion = new AtomicReference<>();
      private final AtomicReference<DimensionHandle> dimension = new AtomicReference<>();
      private AtomicBoolean moveOutOfScope;
      private final AtomicReference<Throwable> callbackFailure;

      private Recorder(AtomicReference<Throwable> callbackFailure) {
         this.callbackFailure = callbackFailure;
      }

      private void configureExpectedAttribute(AttributeHandle attribute) {
         expectedAttribute.set(attribute);
      }

      private void configureDiscoveryScopeChange(RTIambassador ambassador, RegionHandle region,
            DimensionHandle scopeDimension, AtomicBoolean move) {
         scopeOwner.set(ambassador); subscriptionRegion.set(region);
         dimension.set(scopeDimension); moveOutOfScope = move;
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
                  if (moveOutOfScope.get() && discoveries.size() == 2) try {
                     RTIambassador ambassador = scopeOwner.get();
                     RegionHandle region = subscriptionRegion.get();
                     DimensionHandle scopeDimension = dimension.get();
                     ambassador.setRangeBounds(region, scopeDimension,
                        new hla.rti1516_2025.RangeBounds(4, 5));
                     RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
                     regions.add(region);
                     ambassador.commitRegionModifications(regions);
                  } catch (Throwable failure) {
                     callbackFailure.compareAndSet(null, failure);
                  }
               } else if ("provideAttributeValueUpdate".equals(name)
                     && args != null && args.length == 3) {
                  Request request = new Request();
                  request.object = (ObjectInstanceHandle) args[0];
                  request.attributes = (AttributeHandleSet) ((AttributeHandleSet) args[1]).clone();
                  request.tag = ((byte[]) args[2]).clone();
                  requests.add(request);
                  AttributeHandle expected = expectedAttribute.get();
                  if (expected != null && (request.attributes.size() != 1
                        || !request.attributes.contains(expected))) {
                     callbackFailure.compareAndSet(null,
                        new IllegalStateException("regional Auto Provide requested an unexpected attribute"));
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional Auto Provide overlap callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
