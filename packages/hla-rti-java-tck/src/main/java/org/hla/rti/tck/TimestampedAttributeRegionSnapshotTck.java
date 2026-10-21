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
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks callback-scoped stability of conveyed region designators on a TSO reflection. */
final class TimestampedAttributeRegionSnapshotTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedAttributeRegionSnapshotTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String dimensionName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "Callback-scoped region mutation requires HLA_EVOKED callbacks");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-tso-region-snapshot-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-snapshot-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-snapshot-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         DimensionHandle publisherDimension = publisher.getDimensionHandle(dimensionName);
         DimensionHandle receiverDimension = receiver.getDimensionHandle(dimensionName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherAttribute != null && receiverAttribute != null
               && publisherDimension != null && receiverDimension != null
               && publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute)
               && publisherDimension.equals(receiverDimension),
            "Snapshot adapter FOM did not resolve stable class, attribute, and dimension handles");

         AttributeHandleSet publisherAttributes = publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         AttributeHandleSet receiverAttributes = receiver.getAttributeHandleSetFactory().create();
         receiverAttributes.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);

         DimensionHandleSet publisherDimensions = publisher.getDimensionHandleSetFactory().create();
         publisherDimensions.add(publisherDimension);
         DimensionHandleSet receiverDimensions = receiver.getDimensionHandleSetFactory().create();
         receiverDimensions.add(receiverDimension);
         RegionHandle sourceRegion = publisher.createRegion(publisherDimensions);
         RegionHandle subscriptionRegion = receiver.createRegion(receiverDimensions);
         JavaTckSupport.check(sourceRegion != null && subscriptionRegion != null,
            "Snapshot region creation returned no handle");
         publisher.setRangeBounds(sourceRegion, publisherDimension, new RangeBounds(1, 3));
         publisher.commitRegionModifications(regions(publisher, sourceRegion));
         receiver.setRangeBounds(subscriptionRegion, receiverDimension, new RangeBounds(1, 3));
         receiver.commitRegionModifications(regions(receiver, subscriptionRegion));

         AttributeSetRegionSetPairList publisherPair = association(
            publisher, publisherAttributes, sourceRegion);
         AttributeSetRegionSetPairList receiverPair = association(
            receiver, receiverAttributes, subscriptionRegion);
         ObjectInstanceHandle object = publisher.registerObjectInstanceWithRegions(
            publisherClass, publisherPair);
         JavaTckSupport.check(object != null,
            "Snapshot regional object registration returned no handle");
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         await(receiver, () -> receiverCallbacks.discoveries.contains(object),
            "snapshot regional object discovery");

         receiverCallbacks.reflectionHook = event -> {
            JavaTckSupport.check(event.regions != null && event.regions.size() == 1,
               "Timed reflection did not convey exactly one region designator");
            RegionHandle conveyed = event.regions.iterator().next();
            receiverCallbacks.conveyedRegion = conveyed;
            receiverCallbacks.conveyedDimensions = receiver.getDimensionHandleSet(conveyed);
            receiverCallbacks.boundsBeforeMutation = receiver.getRangeBounds(
               conveyed, receiverDimension);
            publisher.setRangeBounds(sourceRegion, publisherDimension, new RangeBounds(0, 2));
            publisher.commitRegionModifications(regions(publisher, sourceRegion));
            receiverCallbacks.boundsAfterMutation = receiver.getRangeBounds(
               conveyed, receiverDimension);
         };

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "Snapshot members selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "Snapshot members began at different logical times");
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         publisher.enableTimeRegulation(publisherEpsilon);
         await(publisher, () -> publisherCallbacks.regulationEnabled == 1,
            "snapshot publisher regulation callback");
         receiver.enableTimeConstrained();
         await(receiver, () -> receiverCallbacks.constrainedEnabled == 1,
            "snapshot receiver constrained callback");

         LogicalTime<?, ?> timestamp = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime<?, ?> receiverTarget = offset(receiverInitial, receiverEpsilon, 5);
         JavaTckSupport.check(timestamp.equals(receiverTarget),
            "Snapshot members calculated different delivery times");
         receiver.timeAdvanceRequest(receiverTarget);
         byte[] value = {0x31, 0x42};
         byte[] tag = {0x52, 0x53, 0x4e};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value.clone());
         MessageRetractionReturn retraction = publisher.updateAttributeValues(
            object, values, tag.clone(), timestamp);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "Snapshot timestamped regional update returned no valid retraction handle");
         publisher.timeAdvanceRequest(timestamp);
         await(publisher, () -> publisherCallbacks.grants.size() == 1,
            "snapshot publisher time advance");
         await(receiver, () -> receiverCallbacks.reflectionComplete,
            "callback-time conveyed-region query and source-region mutation");
         if (receiverCallbacks.callbackFailure != null) {
            throw new AssertionError("Snapshot callback query or mutation failed",
               receiverCallbacks.callbackFailure);
         }
         await(receiver, () -> receiverCallbacks.grants.size() == 1,
            "snapshot receiver grant after reflection");

         JavaTckSupport.check(receiverCallbacks.reflections.size() == 1
               && receiverCallbacks.reflectionCount == 1,
            "Snapshot update did not produce exactly one timed reflection");
         Reflection event = receiverCallbacks.reflections.get(0);
         JavaTckSupport.check(object.equals(event.object)
               && event.values.size() == 1
               && Arrays.equals(value, event.values.get(receiverAttribute))
               && Arrays.equals(tag, event.tag)
               && publisherHandle.equals(event.producer)
               && timestamp.equals(event.time)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(event.retraction)
               && event.regions != null && event.regions.size() == 1,
            "Snapshot reflection changed value, time, order, or region metadata");
         RegionHandle conveyed = receiverCallbacks.conveyedRegion;
         DimensionHandleSet conveyedDimensions = receiverCallbacks.conveyedDimensions;
         RangeBounds before = receiverCallbacks.boundsBeforeMutation;
         RangeBounds after = receiverCallbacks.boundsAfterMutation;
         JavaTckSupport.check(conveyed != null && event.regions.contains(conveyed)
               && conveyedDimensions != null && conveyedDimensions.size() == 1
               && conveyedDimensions.contains(receiverDimension)
               && before != null && before.lower == 1 && before.upper == 3
               && after != null && after.lower == 1 && after.upper == 3,
            "Conveyed region bounds changed after its source region was modified");
         RangeBounds sourceBounds = publisher.getRangeBounds(sourceRegion, publisherDimension);
         JavaTckSupport.check(sourceBounds.lower == 0 && sourceBounds.upper == 2,
            "Publisher source region did not retain its committed post-send bounds");

         receiver.disableTimeConstrained();
         publisher.disableTimeRegulation();
         receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         publisher.unassociateRegionsForUpdates(object, publisherPair);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         receiver.deleteRegion(subscriptionRegion);
         publisher.deleteRegion(sourceRegion);
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static RegionHandleSet regions(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions(ambassador, region)));
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface ReflectionHook { void accept(Reflection event) throws Exception; }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         regions = arguments[5] == null ? null : ((RegionHandleSet) arguments[5]).clone();
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private volatile int reflectionCount;
      private volatile boolean reflectionComplete;
      private volatile Throwable callbackFailure;
      private volatile ReflectionHook reflectionHook;
      private volatile RegionHandle conveyedRegion;
      private volatile DimensionHandleSet conveyedDimensions;
      private volatile RangeBounds boundsBeforeMutation;
      private volatile RangeBounds boundsAfterMutation;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  regulationEnabled++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  constrainedEnabled++;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  try {
                     Reflection event = new Reflection(arguments);
                     reflections.add(event);
                     reflectionCount++;
                     ReflectionHook hook = reflectionHook;
                     if (hook != null) hook.accept(event);
                  } catch (Throwable failure) {
                     callbackFailure = failure;
                  } finally {
                     reflectionComplete = true;
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK region snapshot callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
