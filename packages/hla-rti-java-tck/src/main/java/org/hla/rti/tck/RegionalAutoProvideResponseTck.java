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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicReference;

/** Exercises automatic response to a scoped regional value request during discovery. */
final class RegionalAutoProvideResponseTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] RESPONSE_VALUE = new byte[] {0x52, 0x45, 0x53};
   private static final byte[] RESPONSE_TAG = new byte[] {0x41, 0x50, 0x52};
   private RegionalAutoProvideResponseTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String dimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      List<String> callbackOrder = new CopyOnWriteArrayList<>();
      Recorder ownerCallbacks = new Recorder(callbackOrder);
      Recorder requesterCallbacks = new Recorder(callbackOrder);
      AtomicReference<Throwable> callbackFailure = new AtomicReference<>();
      String federation = "java-tck-regional-auto-provide-response-" + UUID.randomUUID();
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
         ownerCallbacks.configureAutoProvide(owner, callbackFailure);
         owner.connect(ownerCallbacks.proxy(), callbackModel); ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel); requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "regional-auto-provide-owner", "owner", federation); ownerJoined = true;
         requester.joinFederationExecution("regional-auto-provide-requester", "requester", federation);
         requesterJoined = true;

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
         AttributeHandleValueMap autoProvideValues =
            owner.getAttributeHandleValueMapFactory().create(1);
         autoProvideValues.put(ownerAttribute, RESPONSE_VALUE.clone());
         ownerCallbacks.configureResponseValues(autoProvideValues);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes); published = true;
         ownerRegion = createRegion(owner, ownerDimension, 0, 2); ownerRegionCreated = true;
         requesterRegion = createRegion(requester, requesterDimension, 1, 3);
         requesterRegionCreated = true;
         ownerPair = association(owner, ownerAttributes, ownerRegion);
         requesterPair = association(requester, requesterAttributes, requesterRegion);
         ownerCallbacks.configureExpectedAttribute(ownerAttribute);
         requester.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(requester.getConveyRegionDesignatorSetsSwitch(),
            "regional Auto Provide requester did not enable region-designator callbacks");

         object = owner.registerObjectInstanceWithRegions(ownerClass, ownerPair);
         associated = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional Auto Provide registration returned an invalid handle");
         requester.subscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         subscribed = true;
         ObjectInstanceHandle registeredObject = object;
         await(owner, requester, callbackModel,
            () -> requesterCallbacks.discoveries.size() == 1
               && ownerCallbacks.provided.size() == 1
               && requesterCallbacks.reflections.size() == 1,
            "regional Auto Provide discovery, provider callback, and reflection");
         if (callbackFailure.get() != null) {
            throw new AssertionError("regional Auto Provide callback failed", callbackFailure.get());
         }
         JavaTckSupport.check(callbackOrder.equals(Arrays.asList("discover", "provide", "reflect")),
            "regional Auto Provide callback order was not discovery, provide, reflection");

         Discovery discovery = requesterCallbacks.discoveries.get(0);
         JavaTckSupport.check(registeredObject.equals(discovery.object)
               && requesterClass.equals(discovery.objectClass)
               && producer.equals(discovery.producer),
            "regional Auto Provide discovery returned incorrect object metadata");
         ProvidedUpdate provided = ownerCallbacks.provided.get(0);
         JavaTckSupport.check(registeredObject.equals(provided.object)
               && provided.attributes.size() == 1 && provided.attributes.contains(ownerAttribute)
               && provided.tag.length == 0,
            "regional Auto Provide request changed object, attribute, or empty request tag");
         Reflection reflection = requesterCallbacks.reflections.get(0);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(registeredObject.equals(reflection.object)
               && reflection.values.size() == 1
               && Arrays.equals(RESPONSE_VALUE, reflection.values.get(requesterAttribute))
               && Arrays.equals(RESPONSE_TAG, reflection.tag)
               && producer.equals(reflection.producer),
            "regional Auto Provide reflection changed object, value, tag, or producer");
         JavaTckSupport.check(reflection.transportation != null
               && reliable.equals(reflection.transportation)
               && reflection.regions != null && reflection.regions.size() == 1
               && reflection.regions.contains(ownerRegion),
            "regional Auto Provide reflection lost transport or source-region metadata");

         requester.unsubscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         subscribed = false;
         owner.unassociateRegionsForUpdates(object, ownerPair); associated = false;
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
         "adapter Auto Provide object class must expose exactly its configured DDM dimension");
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

   private static final class ProvidedUpdate {
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
      private final List<String> callbackOrder;
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final AtomicReference<RTIambassador> autoProvideAmbassador = new AtomicReference<>();
      private final AtomicReference<AttributeHandle> autoProvideAttribute = new AtomicReference<>();
      private volatile AttributeHandleValueMap autoProvideValues;
      private volatile AtomicReference<Throwable> callbackFailure;
      private Recorder(List<String> callbackOrder) { this.callbackOrder = callbackOrder; }
      private void configureAutoProvide(RTIambassador ambassador, AtomicReference<Throwable> failure) {
         autoProvideAmbassador.set(ambassador);
         callbackFailure = failure;
      }
      private void configureExpectedAttribute(AttributeHandle attribute) {
         autoProvideAttribute.set(attribute);
      }
      private void configureResponseValues(AttributeHandleValueMap values) {
         autoProvideValues = values;
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
                  discoveries.add(discovery); callbackOrder.add("discover");
               } else if ("provideAttributeValueUpdate".equals(name) && args != null
                     && args.length == 3) {
                  ProvidedUpdate update = new ProvidedUpdate();
                  update.object = (ObjectInstanceHandle) args[0];
                  update.attributes = (AttributeHandleSet) ((AttributeHandleSet) args[1]).clone();
                  update.tag = ((byte[]) args[2]).clone();
                  provided.add(update); callbackOrder.add("provide");
                  try {
                     AttributeHandle expected = autoProvideAttribute.get();
                     if (update.attributes.size() != 1 || !update.attributes.contains(expected)) {
                        throw new IllegalStateException("regional Auto Provide requested an unexpected attribute");
                     }
                     autoProvideAmbassador.get().updateAttributeValues(
                        update.object, autoProvideValues, RESPONSE_TAG.clone());
                  } catch (Throwable failure) {
                     AtomicReference<Throwable> destination = callbackFailure;
                     if (destination != null) destination.compareAndSet(null, failure);
                  }
               } else if ("reflectAttributeValues".equals(name) && args != null
                     && args.length == 6) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) args[0];
                  reflection.values = ((AttributeHandleValueMap) args[1]).clone();
                  reflection.tag = ((byte[]) args[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) args[3];
                  reflection.producer = (FederateHandle) args[4];
                  reflection.regions = (RegionHandleSet) args[5];
                  reflections.add(reflection); callbackOrder.add("reflect");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional Auto Provide callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
