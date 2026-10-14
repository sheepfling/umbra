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

/** Verifies regional Auto Provide follows attribute ownership across providers. */
final class RegionalAutoProvideMultiProviderTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] DIVESTITURE_TAG = new byte[] {(byte) 0xD1, 0x4D, 0x25};
   private static final byte[] ACQUISITION_TAG = new byte[] {(byte) 0xA1, 0x4D, 0x25};
   private static final byte[] OWNER_VALUE = new byte[] {0x41, 0x2D, 0x52, 0x45, 0x47};
   private static final byte[] PROVIDER_VALUE = new byte[] {0x42, 0x2D, 0x52, 0x45, 0x47};
   private static final byte[] OWNER_TAG = new byte[] {0x54, 0x41};
   private static final byte[] PROVIDER_TAG = new byte[] {0x54, 0x42};

   private RegionalAutoProvideMultiProviderTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador provider = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      AtomicReference<Throwable> callbackFailure = new AtomicReference<>();
      Recorder ownerCallbacks = new Recorder(callbackFailure, Role.OWNER);
      Recorder providerCallbacks = new Recorder(callbackFailure, Role.PROVIDER);
      Recorder requesterCallbacks = new Recorder(callbackFailure, Role.REQUESTER);
      String federation = "java-tck-regional-auto-provide-multi-provider-" + UUID.randomUUID();
      boolean ownerConnected = false, providerConnected = false, requesterConnected = false;
      boolean created = false, ownerJoined = false, providerJoined = false, requesterJoined = false;
      boolean ownerPublished = false, providerPublished = false;
      boolean providerSubscribed = false, requesterSubscribed = false, ownerAssociated = false;
      boolean ownerRegionCreated = false, providerRegionCreated = false, requesterRegionCreated = false;
      ObjectClassHandle ownerClass = null, providerClass = null, requesterClass = null;
      AttributeHandle ownerA = null, ownerB = null, providerB = null, requesterA = null, requesterB = null;
      AttributeHandleSet ownerAttributes = null, providerAttributes = null, requesterAttributes = null;
      AttributeHandleSet ownerASet = null;
      AttributeSetRegionSetPairList ownerPair = null, ownerARegionPair = null;
      AttributeSetRegionSetPairList providerPair = null, requesterPair = null;
      RegionHandle ownerRegion = null, providerRegion = null, requesterRegion = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel); ownerConnected = true;
         provider.connect(providerCallbacks.proxy(), callbackModel); providerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel); requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "regional-auto-provide-multi-provider-owner", "owner", federation); ownerJoined = true;
         FederateHandle providerHandle = provider.joinFederationExecution(
            "regional-auto-provide-multi-provider-source", "provider", federation);
         providerJoined = true;
         requester.joinFederationExecution("regional-auto-provide-multi-provider-requester",
            "requester", federation); requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         ownerA = owner.getAttributeHandle(ownerClass, firstAttributeName);
         ownerB = owner.getAttributeHandle(ownerClass, secondAttributeName);
         DimensionHandle ownerDimension = owner.getDimensionHandle(dimensionName);
         providerClass = provider.getObjectClassHandle(objectClassName);
         providerB = provider.getAttributeHandle(providerClass, secondAttributeName);
         DimensionHandle providerDimension = provider.getDimensionHandle(dimensionName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         requesterA = requester.getAttributeHandle(requesterClass, firstAttributeName);
         requesterB = requester.getAttributeHandle(requesterClass, secondAttributeName);
         DimensionHandle requesterDimension = requester.getDimensionHandle(dimensionName);
         JavaTckSupport.check(ownerClass.equals(providerClass) && ownerClass.equals(requesterClass)
               && ownerA.equals(requesterA) && ownerB.equals(providerB) && ownerB.equals(requesterB)
               && ownerDimension.equals(providerDimension)
               && ownerDimension.equals(requesterDimension),
            "adapter Auto Provide FOM returned inconsistent multi-provider handles");
         JavaTckSupport.check(owner.getAutoProvideSwitch() && provider.getAutoProvideSwitch()
               && requester.getAutoProvideSwitch(),
            "adapter FOM did not enable Auto Provide for all multi-provider federates");
         checkDimension(owner, ownerClass, ownerDimension);
         checkDimension(provider, providerClass, providerDimension);
         checkDimension(requester, requesterClass, requesterDimension);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerA, ownerB);
         providerAttributes = JavaTckSupport.attributeSet(provider, providerB);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterA, requesterB);
         ownerASet = JavaTckSupport.attributeSet(owner, ownerA);
         ownerCallbacks.configureProvider(owner, null);
         providerCallbacks.configureProvider(provider, null);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes); ownerPublished = true;
         provider.publishObjectClassAttributes(providerClass, providerAttributes);
         providerPublished = true;

         ownerRegion = createRegion(owner, ownerDimension, 0, 2); ownerRegionCreated = true;
         providerRegion = createRegion(provider, providerDimension, 0, 10);
         providerRegionCreated = true;
         requesterRegion = createRegion(requester, requesterDimension, 1, 3);
         requesterRegionCreated = true;
         ownerPair = association(owner, ownerAttributes, ownerRegion);
         ownerARegionPair = association(owner, ownerASet, ownerRegion);
         providerPair = association(provider, providerAttributes, providerRegion);
         requesterPair = association(requester, requesterAttributes, requesterRegion);
         provider.subscribeObjectClassAttributesWithRegions(providerClass, providerPair);
         providerSubscribed = true;
         requester.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(requester.getConveyRegionDesignatorSetsSwitch(),
            "multi-provider requester did not enable region-designator callbacks");

         object = owner.registerObjectInstanceWithRegions(ownerClass, ownerPair);
         ownerAssociated = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "multi-provider regional registration returned an invalid object handle");
         ObjectInstanceHandle registeredObject = object;
         await(owner, provider, requester, callbackModel,
            () -> providerCallbacks.discoveries.size() == 1 && ownerCallbacks.requests.size() == 1,
            "initial provider discovery and owner solicitation");
         checkCallbackFailure(callbackFailure);
         verifyDiscovery(providerCallbacks.discoveries.get(0), registeredObject, providerClass,
            ownerHandle, "second provider");
         verifyRequest(ownerCallbacks.requests.get(0), registeredObject,
            ownerB, "initial owner solicitation");

         owner.unconditionalAttributeOwnershipDivestiture(registeredObject,
            JavaTckSupport.attributeSet(owner, ownerB), DIVESTITURE_TAG.clone());
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(registeredObject, ownerA)
               && !owner.isAttributeOwnedByFederate(registeredObject, ownerB),
            "owner divestiture changed the wrong attribute ownership state");
         await(owner, provider, requester, callbackModel,
            () -> providerCallbacks.assumptions.size() == 1,
            "second provider ownership-assumption callback");
         checkOwnershipEvent(providerCallbacks.assumptions.get(0), registeredObject,
            providerAttributes, DIVESTITURE_TAG, "ownership offer");

         provider.attributeOwnershipAcquisitionIfAvailable(registeredObject, providerAttributes,
            ACQUISITION_TAG.clone());
         await(owner, provider, requester, callbackModel,
            () -> providerCallbacks.acquisitions.size() == 1,
            "second provider ownership-acquisition callback");
         checkOwnershipEvent(providerCallbacks.acquisitions.get(0), registeredObject,
            providerAttributes, ACQUISITION_TAG, "ownership acquisition");
         JavaTckSupport.check(provider.isAttributeOwnedByFederate(registeredObject, providerB),
            "second provider did not become owner of the transferred attribute");

         ownerCallbacks.configureProvider(owner, ownerA);
         providerCallbacks.configureProvider(provider, providerB);
         requester.subscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         requesterSubscribed = true;
         await(owner, provider, requester, callbackModel,
            () -> requesterCallbacks.discoveries.size() == 1
               && ownerCallbacks.requests.size() == 2
               && providerCallbacks.requests.size() == 1
               && requesterCallbacks.reflections.size() == 2,
            "per-owner regional Auto Provide fan-out and reflections");
         checkCallbackFailure(callbackFailure);
         verifyDiscovery(requesterCallbacks.discoveries.get(0), registeredObject, requesterClass,
            ownerHandle, "requester");

         JavaTckSupport.check(ownerCallbacks.requests.size() == 2,
            "original owner received an unexpected request count");
         verifyRequest(ownerCallbacks.requests.get(0), registeredObject, ownerB,
            "pre-transfer solicitation");
         verifyRequest(ownerCallbacks.requests.get(1), registeredObject, ownerA,
            "post-transfer original-owner solicitation");
         verifyRequest(providerCallbacks.requests.get(0), registeredObject, providerB,
            "post-transfer new-owner solicitation");

         TransportationTypeHandle reliable = owner.getTransportationTypeHandle("HLAreliable");
         boolean sawOwnerReflection = false, sawProviderReflection = false;
         for (Reflection reflection : requesterCallbacks.reflections) {
            JavaTckSupport.check(registeredObject.equals(reflection.object)
                  && reflection.values.size() == 1 && reliable.equals(reflection.transportation)
                  && reflection.regions != null,
               "multi-provider reflection changed common object or transport metadata");
            if (ownerHandle.equals(reflection.producer)) {
               JavaTckSupport.check(!sawOwnerReflection
                     && Arrays.equals(OWNER_VALUE, reflection.values.get(requesterA))
                     && Arrays.equals(OWNER_TAG, reflection.tag)
                     && reflection.regions.size() == 1
                     && reflection.regions.contains(ownerRegion),
                  "original-owner reflection changed value, tag, or source-region metadata");
               sawOwnerReflection = true;
            } else if (providerHandle.equals(reflection.producer)) {
               JavaTckSupport.check(!sawProviderReflection
                     && Arrays.equals(PROVIDER_VALUE, reflection.values.get(requesterB))
                     && Arrays.equals(PROVIDER_TAG, reflection.tag)
                     && reflection.regions.isEmpty(),
                  "new-owner reflection changed value, tag, or default region designator");
               sawProviderReflection = true;
            } else {
               throw new AssertionError("multi-provider reflection had an unexpected producer");
            }
         }
         JavaTckSupport.check(sawOwnerReflection && sawProviderReflection,
            "regional Auto Provide did not reflect once from each attribute owner");

         requester.unsubscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         requesterSubscribed = false;
         provider.unsubscribeObjectClassAttributesWithRegions(providerClass, providerPair);
         providerSubscribed = false;
         owner.unassociateRegionsForUpdates(registeredObject, ownerARegionPair);
         ownerAssociated = false;
         deleteRegion(requester, requesterRegion, requesterRegionCreated); requesterRegionCreated = false;
         deleteRegion(provider, providerRegion, providerRegionCreated); providerRegionCreated = false;
         deleteRegion(owner, ownerRegion, ownerRegionCreated); ownerRegionCreated = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION); requesterJoined = false;
         provider.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         providerJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation); created = false;
      } finally {
         if (requesterJoined && requesterSubscribed && requesterClass != null && requesterPair != null) try {
            requester.unsubscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         } catch (Exception ignored) { }
         if (providerJoined && providerSubscribed && providerClass != null && providerPair != null) try {
            provider.unsubscribeObjectClassAttributesWithRegions(providerClass, providerPair);
         } catch (Exception ignored) { }
         if (ownerJoined && ownerAssociated && object != null && ownerARegionPair != null) try {
            owner.unassociateRegionsForUpdates(object, ownerARegionPair);
         } catch (Exception ignored) { }
         if (providerJoined && providerPublished && providerClass != null && providerAttributes != null) try {
            provider.unpublishObjectClassAttributes(providerClass, providerAttributes);
         } catch (Exception ignored) { }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (requesterJoined) deleteRegion(requester, requesterRegion, requesterRegionCreated);
         if (providerJoined) deleteRegion(provider, providerRegion, providerRegionCreated);
         if (ownerJoined) deleteRegion(owner, ownerRegion, ownerRegionCreated);
         if (requesterJoined) try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (providerJoined) try {
            provider.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); } catch (Exception ignored) { }
         if (providerConnected) try { provider.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
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

   private static void checkDimension(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle dimension) throws Exception {
      java.util.Set<DimensionHandle> available =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(available.size() == 1 && available.contains(dimension)
            && ambassador.getDimensionUpperBound(dimension) >= 10,
         "adapter Auto Provide object class must expose its configured dimension through range 10");
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

   private static void await(RTIambassador owner, RTIambassador provider, RTIambassador requester,
         CallbackModel model, java.util.function.BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(requester, model); pump(provider, model); pump(owner, model); Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void checkCallbackFailure(AtomicReference<Throwable> failure) {
      if (failure.get() != null) throw new AssertionError("multi-provider Auto Provide callback failed",
         failure.get());
   }

   private static void verifyDiscovery(Discovery discovery, ObjectInstanceHandle object,
         ObjectClassHandle objectClass, FederateHandle producer, String label) {
      JavaTckSupport.check(object.equals(discovery.object) && objectClass.equals(discovery.objectClass)
            && producer.equals(discovery.producer),
         label + " discovery returned incorrect object metadata");
   }

   private static void verifyRequest(Request request, ObjectInstanceHandle object,
         AttributeHandle attribute, String label) {
      JavaTckSupport.check(object.equals(request.object) && request.attributes.size() == 1
            && request.attributes.contains(attribute) && request.tag.length == 0,
         label + " did not target exactly the expected attribute with an empty request tag");
   }

   private static void checkOwnershipEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String label) {
      JavaTckSupport.check(object.equals(event.object) && attributes.equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         label + " changed its object, attributes, or tag");
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean created) {
      if (created && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private enum Role { OWNER, PROVIDER, REQUESTER }

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

   private static final class OwnershipEvent {
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
      private final AtomicReference<Throwable> callbackFailure;
      private final Role role;
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Request> requests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final AtomicReference<RTIambassador> ambassador = new AtomicReference<>();
      private final AtomicReference<AttributeHandle> responseAttribute = new AtomicReference<>();

      private Recorder(AtomicReference<Throwable> callbackFailure, Role role) {
         this.callbackFailure = callbackFailure; this.role = role;
      }

      private void configureProvider(RTIambassador rti, AttributeHandle response) {
         ambassador.set(rti); responseAttribute.set(response);
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
               } else if ("provideAttributeValueUpdate".equals(name) && args != null
                     && args.length == 3) {
                  Request request = new Request();
                  request.object = (ObjectInstanceHandle) args[0];
                  request.attributes = (AttributeHandleSet) ((AttributeHandleSet) args[1]).clone();
                  request.tag = ((byte[]) args[2]).clone();
                  requests.add(request);
                  AttributeHandle expected = responseAttribute.get();
                  if (expected != null) try {
                     respond(request, expected);
                  } catch (Throwable failure) { callbackFailure.compareAndSet(null, failure); }
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && args != null && args.length == 3) {
                  assumptions.add(ownershipEvent(args));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && args != null && args.length == 3) {
                  acquisitions.add(ownershipEvent(args));
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
                  if ("toString".equals(name)) return "Java TCK multi-provider Auto Provide callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }

      private void respond(Request request, AttributeHandle expected) throws Exception {
         if (request.attributes.size() != 1 || !request.attributes.contains(expected)
               || request.tag.length != 0) {
            throw new IllegalStateException(role + " received an unexpected Auto Provide request");
         }
         byte[] value = role == Role.OWNER ? OWNER_VALUE : PROVIDER_VALUE;
         byte[] tag = role == Role.OWNER ? OWNER_TAG : PROVIDER_TAG;
         AttributeHandleValueMap values = ambassador.get().getAttributeHandleValueMapFactory().create(1);
         values.put(expected, value.clone());
         ambassador.get().updateAttributeValues(request.object, values, tag.clone());
      }

      private static OwnershipEvent ownershipEvent(Object[] args) {
         OwnershipEvent event = new OwnershipEvent();
         event.object = (ObjectInstanceHandle) args[0];
         event.attributes = (AttributeHandleSet) ((AttributeHandleSet) args[1]).clone();
         event.tag = ((byte[]) args[2]).clone();
         return event;
      }
   }
}
