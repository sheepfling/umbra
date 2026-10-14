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
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies that passive regional subscriptions suppress discovery and reflection until active. */
final class PassiveRegionalSubscriptionTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;
   private static final byte[] PASSIVE_VALUE = new byte[] {0x50, 0x60};
   private static final byte[] ACTIVE_VALUE = new byte[] {0x70, (byte) 0x80};
   private static final byte[] ACTIVE_TAG = new byte[] {0x50, 0x52};

   private PassiveRegionalSubscriptionTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied dimensional FOM is required");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-passive-regional-subscription-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean sourceRegionCreated = false;
      boolean subscriberRegionCreated = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle subscriberClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandle subscriberAttribute = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeSetRegionSetPairList sourcePair = null;
      AttributeSetRegionSetPairList subscriberPair = null;
      RegionHandle sourceRegion = null;
      RegionHandle subscriberRegion = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle publisherFederate =
            publisher.joinFederationExecution("passive-regional-owner", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("passive-regional-member", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle subscriberFirst = subscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle subscriberSecond = subscriber.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherClass.equals(subscriberClass)
               && publisherAttribute.equals(subscriberAttribute)
               && publisherFirst.equals(subscriberFirst)
               && publisherSecond.equals(subscriberSecond),
            "adapter DDM FOM returned inconsistent object, attribute, or dimension handles");
         checkDimensions(publisher, publisherClass, publisherFirst, publisherSecond);
         checkDimensions(subscriber, subscriberClass, subscriberFirst, subscriberSecond);

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet subscriberAttributes =
            JavaTckSupport.attributeSet(subscriber, subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5);
         sourceRegionCreated = true;
         subscriberRegion = createRegion(subscriber, subscriberFirst, subscriberSecond, 2, 6);
         subscriberRegionCreated = true;
         sourcePair = association(publisher, publisherAttributes, sourceRegion);
         subscriberPair = association(subscriber, subscriberAttributes, subscriberRegion);

         subscriber.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(subscriber.getConveyRegionDesignatorSetsSwitch(),
            "passive regional subscriber did not enable region-designator callbacks");
         subscriber.subscribeObjectClassAttributesPassivelyWithRegions(
            subscriberClass, subscriberPair, "");
         subscribed = true;

         object = publisher.registerObjectInstanceWithRegions(publisherClass, sourcePair);
         JavaTckSupport.check(object != null,
            "passive regional registration returned a null object handle");
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.discoveries.isEmpty()
               && subscriberCallbacks.reflections.isEmpty(),
            "passive regional subscription arranged discovery or reflection");
         ObjectInstanceHandle passivelyRegisteredObject = object;
         JavaTckSupport.expectFailure(
            () -> subscriber.getKnownObjectClassHandle(passivelyRegisteredObject),
            "ObjectInstanceNotKnown",
            "known-object lookup for a passively subscribed regional object");

         AttributeHandleValueMap passiveValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         passiveValues.put(publisherAttribute, PASSIVE_VALUE.clone());
         publisher.updateAttributeValues(object, passiveValues, new byte[0]);
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.reflections.isEmpty(),
            "passive regional subscription received an attribute update");

         subscriber.subscribeObjectClassAttributesWithRegions(
            subscriberClass, subscriberPair, "");
         await(subscriber, callbackModel,
            () -> !subscriberCallbacks.discoveries.isEmpty(),
            "active regional discovery after passive declaration");
         Discovery discovery = subscriberCallbacks.discoveries.get(0);
         JavaTckSupport.check(object.equals(discovery.object)
               && subscriberClass.equals(discovery.objectClass)
               && publisherFederate.equals(discovery.producer),
            "active regional discovery returned incorrect object metadata");

         AttributeHandleValueMap activeValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         activeValues.put(publisherAttribute, ACTIVE_VALUE.clone());
         publisher.updateAttributeValues(object, activeValues, ACTIVE_TAG.clone());
         await(subscriber, callbackModel,
            () -> !subscriberCallbacks.reflections.isEmpty(),
            "active regional reflection after passive declaration");
         Reflection reflection = subscriberCallbacks.reflections.get(0);
         JavaTckSupport.check(object.equals(reflection.object)
               && reflection.values.size() == 1
               && Arrays.equals(ACTIVE_VALUE, reflection.values.get(subscriberAttribute))
               && Arrays.equals(ACTIVE_TAG, reflection.tag)
               && publisherFederate.equals(reflection.producer),
            "active regional reflection changed object, value, tag, or producer");
         JavaTckSupport.check(reflection.transportation != null
               && !subscriber.getTransportationTypeName(reflection.transportation).isEmpty()
               && reflection.regions != null && reflection.regions.size() == 1
               && reflection.regions.contains(sourceRegion),
            "active regional reflection did not preserve source-region metadata");
      } finally {
         if (subscriberJoined && subscribed && subscriberPair != null && subscriberClass != null) {
            try { subscriber.unsubscribeObjectClassAttributesWithRegions(
               subscriberClass, subscriberPair); } catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherClass != null && publisherAttributes != null) {
            try { publisher.unpublishObjectClassAttributes(
               publisherClass, publisherAttributes); } catch (Exception ignored) { }
         }
         if (publisherJoined && sourceRegionCreated && sourceRegion != null) {
            try { publisher.deleteRegion(sourceRegion); } catch (Exception ignored) { }
         }
         if (subscriberJoined && subscriberRegionCreated && subscriberRegion != null) {
            try { subscriber.deleteRegion(subscriberRegion); } catch (Exception ignored) { }
         }
         if (publisherJoined) resign(publisher, ResignAction.DELETE_OBJECTS);
         if (subscriberJoined) resign(subscriber, ResignAction.NO_ACTION);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      JavaTckSupport.check(!first.equals(second)
            && ambassador.getAvailableDimensionsForObjectClass(objectClass).contains(first)
            && ambassador.getAvailableDimensionsForObjectClass(objectClass).contains(second)
            && ambassador.getDimensionUpperBound(first) >= 6
            && ambassador.getDimensionUpperBound(second) >= 6,
         "adapter FOM must expose two distinct object dimensions with bounds through 6");
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
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      pairs.add(new AttributeRegionAssociation(attributes, regions));
      return pairs;
   }

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(ambassador);
      } else {
         long deadline = System.nanoTime() + 100_000_000L;
         while (System.nanoTime() < deadline) Thread.yield();
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final FederateHandle producer;
      private Discovery(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         objectClass = (ObjectClassHandle) arguments[1];
         producer = (FederateHandle) arguments[3];
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap received = (AttributeHandleValueMap) arguments[1];
         values = new LinkedHashMap<>();
         received.forEach((handle, value) -> values.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         RegionHandleSet receivedRegions = (RegionHandleSet) arguments[5];
         regions = receivedRegions == null ? null : (RegionHandleSet) receivedRegions.clone();
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) discoveries.add(new Discovery(arguments));
               else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) reflections.add(new Reflection(arguments));
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK passive regional subscription";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
