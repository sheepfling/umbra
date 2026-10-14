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
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Preserves a queued regional timestamped update across changed-lookahead re-enable. */
final class TimestampedRegionalAttributeRegulationReenableTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedRegionalAttributeRegulationReenableTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      runInternal(factory, fom, timeImplementation, objectClassName, attributeName,
         firstDimensionName, secondDimensionName, callbackModel, false);
   }

   static void runDefaultRegion(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      runInternal(factory, fom, timeImplementation, objectClassName, attributeName,
         firstDimensionName, secondDimensionName, callbackModel, true);
   }

   private static void runInternal(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel, boolean defaultRegion)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-tso-regional-regulation-reenable-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false, published = false;
      boolean subscribed = false, constrained = false, regulating = false;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandle publisherAttribute = null, receiverAttribute = null;
      AttributeHandleSet publisherAttributes = null, receiverAttributes = null;
      AttributeSetRegionSetPairList publisherPair = null, receiverPair = null;
      RegionHandle publisherRegion = null, receiverRegion = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-regional-regulation-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-regional-regulation-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute)
               && publisherFirst.equals(receiverFirst) && publisherSecond.equals(receiverSecond),
            "adapter DDM FOM returned inconsistent class, attribute, or dimension handles");
         checkDimensions(publisher, publisherClass, publisherFirst, publisherSecond);
         checkDimensions(receiver, receiverClass, receiverFirst, receiverSecond);

         publisherAttributes = attributeSet(publisher, publisherAttribute);
         receiverAttributes = attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         if (!defaultRegion) {
            publisherRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 3);
         }
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 4);
         receiverPair = association(receiver, receiverAttributes, receiverRegion);
         if (!defaultRegion) {
            publisherPair = association(publisher, publisherAttributes, publisherRegion);
         }
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "regional regulation receiver did not enable region designators");
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = true;
         object = defaultRegion ? publisher.registerObjectInstance(publisherClass)
            : publisher.registerObjectInstanceWithRegions(publisherClass, publisherPair);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional regulation re-enable returned an invalid object handle");
         ObjectInstanceHandle registeredObject = object;
         await(publisher, receiver, callbackModel,
            () -> receiverCallbacks.discoveries.contains(registeredObject), "regional object discovery");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "regional regulation members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon = (LogicalTimeInterval) receiverFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         constrained = true;
         publisher.enableTimeRegulation(epsilon);
         regulating = true;
         await(publisher, receiver, callbackModel,
            () -> publisherCallbacks.timeRegulationEnabledCount == 1
               && receiverCallbacks.timeConstrainedEnabledCount == 1,
            "initial regional time-role callbacks");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "regional regulation members began at different logical times");
         LogicalTime updateTime = offset(publisherInitial, epsilon, 5);
         LogicalTime receiverUpdateTime = offset(receiverInitial, receiverEpsilon, 5);
         LogicalTime publisherTarget = offset(publisherInitial, epsilon, 2);
         LogicalTime changedLookaheadTime = offset(publisherInitial, epsilon, 3);
         LogicalTimeInterval changedLookahead = intervalSteps(epsilon, 3);
         byte[] value = new byte[] {0x52, 0x47, 0x41, 0x43};
         byte[] tag = new byte[] {0x43, 0x48, 0x47, 0x52};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value.clone());
         MessageRetractionReturn sent =
            publisher.updateAttributeValues(object, values, tag.clone(), updateTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped regional update returned no valid retraction handle");
         pump(receiver, callbackModel);
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty(),
            "regional timestamped reflection arrived before its constrained grant");

         if (!defaultRegion) {
            publisher.setRangeBounds(publisherRegion, publisherFirst,
               new hla.rti1516_2025.RangeBounds(0, 1));
            publisher.setRangeBounds(publisherRegion, publisherSecond,
               new hla.rti1516_2025.RangeBounds(0, 1));
            publisher.commitRegionModifications(regionSet(publisher, publisherRegion));
         }
         publisher.disableTimeRegulation();
         regulating = false;
         publisher.enableTimeRegulation(changedLookahead);
         regulating = true;
         await(publisher, receiver, callbackModel,
            () -> publisherCallbacks.timeRegulationEnabledCount == 2,
            "second Time Regulation enable callback");
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty(),
            "queued regional reflection arrived during Time Regulation re-enable");
         JavaTckSupport.check(changedLookahead.equals(publisher.queryLookahead()),
            "Query Lookahead did not return the changed lookahead");

         receiverCallbacks.callbackOrder.clear();
         publisherCallbacks.callbackOrder.clear();
         receiver.timeAdvanceRequest(receiverUpdateTime);
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty(),
            "queued reflection arrived while the producer boundary was pending");
         publisher.timeAdvanceRequest(publisherTarget);
         await(publisher, receiver, callbackModel,
            () -> receiverCallbacks.reflections.size() >= 1
               && receiverCallbacks.grants.size() >= 1 && publisherCallbacks.grants.size() >= 1,
            "regional update delivery and matching grants");
         JavaTckSupport.check(receiverCallbacks.reflections.size() == 1
               && publisherCallbacks.timeRegulationEnabledCount == 2
               && receiverCallbacks.callbackOrder.equals(Arrays.asList("reflect", "grant")),
            "re-enabled producer delivered the wrong callbacks or delivery order");
         JavaTckSupport.check(updateTime.equals(receiverCallbacks.grants.get(0))
               && publisherTarget.equals(publisherCallbacks.grants.get(0))
               && updateTime.equals(receiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "time-advance grants or logical-time queries returned the wrong times");
         assertReflection(receiver, receiverCallbacks.reflections.get(0), object,
            receiverAttribute, producer, value, tag, updateTime, sent.handle,
            defaultRegion, publisherRegion);
         JavaTckSupport.expectFailure(() -> publisher.retract(sent.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting the regional timestamped update after delivery");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         if (!defaultRegion) publisher.unassociateRegionsForUpdates(object, publisherPair);
         receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         receiver.deleteRegion(receiverRegion); receiverRegion = null;
         if (publisherRegion != null) {
            publisher.deleteRegion(publisherRegion);
            publisherRegion = null;
         }
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined && receiverPair != null) try {
            receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         deleteRegion(publisher, publisherRegion, publisherJoined);
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static AttributeHandleSet attributeSet(RTIambassador ambassador, AttributeHandle handle)
         throws Exception {
      AttributeHandleSet result = ambassador.getAttributeHandleSetFactory().create();
      result.add(handle);
      return result;
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      java.util.Set<DimensionHandle> dimensions =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(dimensions.contains(first) && dimensions.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 4
            && ambassador.getDimensionUpperBound(second) >= 4,
         "adapter FOM object class does not expose the configured DDM dimensions");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle result = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(result, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(result, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.commitRegionModifications(regionSet(ambassador, result));
      return result;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regionSet(ambassador, region)));
      return result;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTimeInterval intervalSteps(LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTimeInterval result = interval;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(interval);
      return result;
   }

   private static void assertReflection(RTIambassador receiver, Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle attribute, FederateHandle producer,
         byte[] value, byte[] tag, LogicalTime<?, ?> time,
         MessageRetractionHandle retraction, boolean defaultRegion,
         RegionHandle expectedRegion) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer)
            && time.equals(reflection.time) && reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(reflection.retraction) && reflection.regions != null
            && (defaultRegion ? reflection.regions.isEmpty()
               : reflection.regions.size() == 1 && reflection.regions.contains(expectedRegion)),
         "regional regulation re-enable returned incorrect value or source-region metadata");
      TransportationTypeHandle transportation = reflection.transportation;
      JavaTckSupport.check(transportation != null
            && !receiver.getTransportationTypeName(transportation).isEmpty(),
         "regional regulation re-enable returned an unresolvable transportation handle");
   }

   private static void await(RTIambassador first, RTIambassador second,
         CallbackModel model, java.util.function.BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         pump(second, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean joined) {
      if (joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private RegionHandleSet regions;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder, receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile int timeRegulationEnabledCount, timeConstrainedEnabledCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabledCount;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  reflection.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflection.tag = ((byte[]) arguments[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) arguments[3];
                  reflection.producer = (FederateHandle) arguments[4];
                  reflection.regions = (RegionHandleSet) arguments[5];
                  reflection.time = (LogicalTime<?, ?>) arguments[6];
                  reflection.sentOrder = (OrderType) arguments[7];
                  reflection.receivedOrder = (OrderType) arguments[8];
                  reflection.retraction = (MessageRetractionHandle) arguments[9];
                  reflections.add(reflection);
                  callbackOrder.add("reflect");
               } else if ("timeAdvanceGrant".equals(name) && arguments != null) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional regulation re-enable callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
