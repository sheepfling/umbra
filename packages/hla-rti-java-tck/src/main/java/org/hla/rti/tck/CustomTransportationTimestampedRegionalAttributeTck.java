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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Timestamped regional attribute delivery, retraction, filtering, and custom transport. */
final class CustomTransportationTimestampedRegionalAttributeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private CustomTransportationTimestampedRegionalAttributeTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String transportationName,
         String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador immediate = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      RTIambassador disjoint = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder immediateCallbacks = new Recorder();
      Recorder constrainedCallbacks = new Recorder();
      Recorder disjointCallbacks = new Recorder();
      String federation = "java-tck-custom-tso-regional-attribute-" + UUID.randomUUID();
      boolean publisherConnected = false, immediateConnected = false;
      boolean constrainedConnected = false, disjointConnected = false;
      boolean created = false, publisherJoined = false, immediateJoined = false;
      boolean constrainedJoined = false, disjointJoined = false, published = false;
      boolean immediateSubscribed = false, constrainedSubscribed = false;
      boolean disjointSubscribed = false, regulationEnabled = false;
      boolean constrainedEnabled = false;
      ObjectClassHandle publisherClass = null, immediateClass = null;
      ObjectClassHandle constrainedClass = null, disjointClass = null;
      AttributeHandle publisherAttribute = null, immediateAttribute = null;
      AttributeHandle constrainedAttribute = null, disjointAttribute = null;
      AttributeHandleSet publisherAttributes = null, immediateAttributes = null;
      AttributeHandleSet constrainedAttributes = null, disjointAttributes = null;
      AttributeSetRegionSetPairList publisherPair = null, immediatePair = null;
      AttributeSetRegionSetPairList constrainedPair = null, disjointPair = null;
      RegionHandle sourceRegion = null, immediateRegion = null;
      RegionHandle constrainedRegion = null, disjointRegion = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         immediate.connect(immediateCallbacks.proxy(), callbackModel);
         immediateConnected = true;
         constrained.connect(constrainedCallbacks.proxy(), callbackModel);
         constrainedConnected = true;
         disjoint.connect(disjointCallbacks.proxy(), callbackModel);
         disjointConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-tso-regional-attribute-publisher", federation);
         publisherJoined = true;
         immediate.joinFederationExecution("java-tck-tso-regional-attribute-immediate", federation);
         immediateJoined = true;
         constrained.joinFederationExecution(
            "java-tck-tso-regional-attribute-constrained", federation);
         constrainedJoined = true;
         disjoint.joinFederationExecution("java-tck-tso-regional-attribute-disjoint", federation);
         disjointJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         immediateClass = immediate.getObjectClassHandle(objectClassName);
         constrainedClass = constrained.getObjectClassHandle(objectClassName);
         disjointClass = disjoint.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         immediateAttribute = immediate.getAttributeHandle(immediateClass, attributeName);
         constrainedAttribute = constrained.getAttributeHandle(constrainedClass, attributeName);
         disjointAttribute = disjoint.getAttributeHandle(disjointClass, attributeName);
         TransportationTypeHandle expectedTransportation =
            publisher.getTransportationTypeHandle(transportationName);
         check(objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(immediate.getObjectClassName(immediateClass))
               && objectClassName.equals(constrained.getObjectClassName(constrainedClass))
               && objectClassName.equals(disjoint.getObjectClassName(disjointClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass, publisherAttribute))
               && attributeName.equals(immediate.getAttributeName(immediateClass, immediateAttribute))
               && attributeName.equals(constrained.getAttributeName(
                  constrainedClass, constrainedAttribute))
               && attributeName.equals(disjoint.getAttributeName(disjointClass, disjointAttribute))
               && transportationName.equals(publisher.getTransportationTypeName(expectedTransportation)),
            "adapter FOM object, attribute, or custom transportation names did not round-trip");

         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle immediateFirst = immediate.getDimensionHandle(firstDimensionName);
         DimensionHandle immediateSecond = immediate.getDimensionHandle(secondDimensionName);
         DimensionHandle constrainedFirst = constrained.getDimensionHandle(firstDimensionName);
         DimensionHandle constrainedSecond = constrained.getDimensionHandle(secondDimensionName);
         DimensionHandle disjointFirst = disjoint.getDimensionHandle(firstDimensionName);
         DimensionHandle disjointSecond = disjoint.getDimensionHandle(secondDimensionName);
         check(publisherFirst.equals(immediateFirst) && publisherFirst.equals(constrainedFirst)
               && publisherFirst.equals(disjointFirst) && publisherSecond.equals(immediateSecond)
               && publisherSecond.equals(constrainedSecond) && publisherSecond.equals(disjointSecond),
            "adapter DDM FOM returned inconsistent dimension handles");
         checkDimensions(publisher, publisherClass, publisherFirst, publisherSecond);
         checkDimensions(immediate, immediateClass, immediateFirst, immediateSecond);
         checkDimensions(constrained, constrainedClass, constrainedFirst, constrainedSecond);
         checkDimensions(disjoint, disjointClass, disjointFirst, disjointSecond);

         publisherAttributes = attributes(publisher, publisherAttribute);
         immediateAttributes = attributes(immediate, immediateAttribute);
         constrainedAttributes = attributes(constrained, constrainedAttribute);
         disjointAttributes = attributes(disjoint, disjointAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(publisherClass, publisherAttributes,
            OrderType.TIMESTAMP);
         RegionHandleSet sourceRegions;
         RegionHandleSet immediateRegions;
         RegionHandleSet constrainedRegions;
         RegionHandleSet disjointRegions;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5, 1, 5);
         immediateRegion = createRegion(immediate, immediateFirst, immediateSecond, 2, 6, 2, 6);
         constrainedRegion = createRegion(
            constrained, constrainedFirst, constrainedSecond, 2, 6, 2, 6);
         disjointRegion = createRegion(disjoint, disjointFirst, disjointSecond, 7, 9, 7, 9);
         sourceRegions = regionSet(publisher, sourceRegion);
         immediateRegions = regionSet(immediate, immediateRegion);
         constrainedRegions = regionSet(constrained, constrainedRegion);
         disjointRegions = regionSet(disjoint, disjointRegion);
         publisherPair = association(publisher, publisherAttributes, sourceRegions);
         immediatePair = association(immediate, immediateAttributes, immediateRegions);
         constrainedPair = association(constrained, constrainedAttributes, constrainedRegions);
         disjointPair = association(disjoint, disjointAttributes, disjointRegions);
         immediate.setConveyRegionDesignatorSetsSwitch(true);
         constrained.setConveyRegionDesignatorSetsSwitch(true);
         disjoint.setConveyRegionDesignatorSetsSwitch(true);
         check(immediate.getConveyRegionDesignatorSetsSwitch()
               && constrained.getConveyRegionDesignatorSetsSwitch()
               && disjoint.getConveyRegionDesignatorSetsSwitch(),
            "regional attribute receivers did not enable region designator callbacks");
         immediate.subscribeObjectClassAttributesWithRegions(immediateClass, immediatePair);
         immediateSubscribed = true;
         constrained.subscribeObjectClassAttributesWithRegions(constrainedClass, constrainedPair);
         constrainedSubscribed = true;
         disjoint.subscribeObjectClassAttributesWithRegions(disjointClass, disjointPair);
         disjointSubscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstanceWithRegions(
            publisherClass, publisherPair);
         check(object != null && object.encodedLength() > 0,
            "timestamped regional attribute registration returned an invalid handle");
         awaitBoth(publisher, immediate, callbackModel,
            () -> immediateCallbacks.discoveries.contains(object), "immediate object discovery");
         awaitBoth(publisher, constrained, callbackModel,
            () -> constrainedCallbacks.discoveries.contains(object), "constrained object discovery");
         pump(disjoint, callbackModel);
         check(!disjointCallbacks.discoveries.contains(object),
            "disjoint regional subscription discovered the object");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> constrainedFactory = constrained.getTimeFactory();
         check(publisherFactory.getName().equals(constrainedFactory.getName()),
            "timestamped regional participants selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         constrained.enableTimeConstrained();
         constrainedEnabled = true;
         publisher.enableTimeRegulation(epsilon);
         regulationEnabled = true;
         awaitBoth(publisher, constrained, callbackModel,
            () -> publisherCallbacks.timeRegulationEnabled
               && constrainedCallbacks.timeConstrainedEnabledCount >= 1,
            "time regulation/constrained enablement");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime constrainedInitial = constrained.queryLogicalTime();
         check(publisherInitial.equals(constrainedInitial),
            "timestamped regional participants began at different logical times");
         LogicalTime firstTime = addSteps(publisherInitial, epsilon, 3);
         LogicalTime firstConstrainedTime = addSteps(constrainedInitial, epsilon, 3);
         byte[] firstValue = new byte[] {0x01, 0x02, 0x03};
         byte[] firstTag = new byte[] {0x52, 0x31};
         MessageRetractionReturn first = update(
            publisher, object, publisherAttribute, firstValue, firstTag, firstTime);
         requireRetraction(first, "first timestamped regional attribute update");
         await(publisher, immediate, callbackModel,
            () -> immediateCallbacks.reflections.size() == 1, "immediate first reflection");
         assertReflection(immediate, immediateCallbacks.reflections.get(0), object,
            immediateAttribute, producer, firstValue, firstTag, firstTime, OrderType.RECEIVE,
            first.handle, sourceRegion, transportationName);
         check(constrainedCallbacks.reflections.isEmpty(),
            "constrained receiver received a regional update before advancing");
         pump(disjoint, callbackModel);
         check(disjointCallbacks.reflections.isEmpty(),
            "disjoint receiver got a timestamped regional reflection");
         publisher.retract(first.handle);
         await(publisher, immediate, callbackModel,
            () -> immediateCallbacks.retractions.size() == 1, "requestRetraction callback");
         check(first.handle.equals(immediateCallbacks.retractions.get(0)),
            "requestRetraction returned the wrong handle");
         check(constrainedCallbacks.retractions.isEmpty(),
            "pending constrained update produced a retraction callback");
         constrained.disableTimeConstrained();
         constrained.enableTimeConstrained();
         await(publisher, constrained, callbackModel,
            () -> constrainedCallbacks.timeConstrainedEnabledCount >= 2,
            "time-constrained re-enable callback");
         constrained.timeAdvanceRequest(firstConstrainedTime);
         publisher.timeAdvanceRequest(firstTime);
         awaitBoth(publisher, constrained, callbackModel,
            () -> publisherCallbacks.grants.size() == 1 && constrainedCallbacks.grants.size() == 1,
            "first matching time grants");
         pump(constrained, callbackModel);
         check(constrainedCallbacks.reflections.isEmpty()
               && constrainedCallbacks.retractions.isEmpty(),
            "retracted update produced a stale callback at the constrained grant");

         LogicalTime secondTime = addSteps(firstTime, epsilon, 3);
         LogicalTime secondConstrainedTime = addSteps(firstConstrainedTime, epsilon, 3);
         byte[] secondValue = new byte[] {0x21, 0x22, 0x23};
         byte[] secondTag = new byte[] {0x52, 0x32};
         MessageRetractionReturn second = update(
            publisher, object, publisherAttribute, secondValue, secondTag, secondTime);
         requireRetraction(second, "second timestamped regional attribute update");
         await(publisher, immediate, callbackModel,
            () -> immediateCallbacks.reflections.size() == 2, "immediate second reflection");
         assertReflection(immediate, immediateCallbacks.reflections.get(1), object,
            immediateAttribute, producer, secondValue, secondTag, secondTime, OrderType.RECEIVE,
            second.handle, sourceRegion, transportationName);
         check(constrainedCallbacks.reflections.isEmpty(),
            "second update reached constrained receiver before its advance");
         constrainedCallbacks.callbackOrder.clear();
         constrained.timeAdvanceRequest(secondConstrainedTime);
         publisher.timeAdvanceRequest(secondTime);
         awaitBoth(publisher, constrained, callbackModel,
            () -> publisherCallbacks.grants.size() == 2 && constrainedCallbacks.grants.size() == 2
               && constrainedCallbacks.reflections.size() == 1,
            "second timestamped reflection and grants");
         assertReflection(constrained, constrainedCallbacks.reflections.get(0), object,
            constrainedAttribute, producer, secondValue, secondTag, secondTime,
            OrderType.TIMESTAMP, second.handle, sourceRegion, transportationName);
         check(constrainedCallbacks.callbackOrder.equals(Arrays.asList("reflect", "grant")),
            "timestamped regional reflection did not precede its matching time-advance grant");
         expectException(() -> publisher.retract(second.handle), "MessageCanNoLongerBeRetracted",
            "retracting delivered regional timestamped update");

         constrained.disableTimeConstrained();
         constrainedEnabled = false;
         publisher.disableTimeRegulation();
         regulationEnabled = false;
         immediate.unsubscribeObjectClassAttributesWithRegions(immediateClass, immediatePair);
         immediateSubscribed = false;
         constrained.unsubscribeObjectClassAttributesWithRegions(constrainedClass, constrainedPair);
         constrainedSubscribed = false;
         disjoint.unsubscribeObjectClassAttributesWithRegions(disjointClass, disjointPair);
         disjointSubscribed = false;
         publisher.unassociateRegionsForUpdates(object, publisherPair);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         publisher.deleteRegion(sourceRegion);
         immediate.deleteRegion(immediateRegion);
         constrained.deleteRegion(constrainedRegion);
         disjoint.deleteRegion(disjointRegion);
      } finally {
         if (constrainedEnabled && constrainedJoined) try { constrained.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulationEnabled && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (immediateSubscribed && immediateJoined) try {
            immediate.unsubscribeObjectClassAttributesWithRegions(immediateClass, immediatePair);
         } catch (Exception ignored) { }
         if (constrainedSubscribed && constrainedJoined) try {
            constrained.unsubscribeObjectClassAttributesWithRegions(constrainedClass, constrainedPair);
         } catch (Exception ignored) { }
         if (disjointSubscribed && disjointJoined) try {
            disjoint.unsubscribeObjectClassAttributesWithRegions(disjointClass, disjointPair);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         if (disjointJoined) resign(disjoint);
         if (constrainedJoined) resign(constrained);
         if (immediateJoined) resign(immediate);
         if (publisherJoined) publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (disjointConnected) disconnect(disjoint);
         if (constrainedConnected) disconnect(constrained);
         if (immediateConnected) disconnect(immediate);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static AttributeHandleSet attributes(RTIambassador ambassador, AttributeHandle handle)
         throws Exception {
      AttributeHandleSet result = ambassador.getAttributeHandleSetFactory().create();
      result.add(handle);
      return result;
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long firstLower, long firstUpper,
         long secondLower, long secondUpper) throws Exception {
      check(ambassador.getDimensionUpperBound(first) >= firstUpper
            && ambassador.getDimensionUpperBound(second) >= secondUpper,
         "adapter DDM dimensions do not cover the requested regions");
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle result = ambassador.createRegion(dimensions);
      RegionHandleSet set = regionSet(ambassador, result);
      ambassador.setRangeBounds(result, first, new hla.rti1516_2025.RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(result, second, new hla.rti1516_2025.RangeBounds(secondLower, secondUpper));
      ambassador.commitRegionModifications(set);
      return result;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandleSet regions) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions));
      return result;
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      java.util.Set<DimensionHandle> dimensions =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      check(dimensions.contains(first) && dimensions.contains(second),
         "adapter object class does not expose both configured dimensions");
   }

   private static MessageRetractionReturn update(RTIambassador ambassador,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value,
         byte[] tag, LogicalTime<?, ?> time) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      return ambassador.updateAttributeValues(object, values, tag.clone(), time);
   }

   private static void assertReflection(RTIambassador receiver, Reflection actual,
         ObjectInstanceHandle object, AttributeHandle attribute, FederateHandle producer,
         byte[] value, byte[] tag, LogicalTime<?, ?> time, OrderType receivedOrder,
         MessageRetractionHandle retraction, RegionHandle sourceRegion,
         String transportationName) throws Exception {
      check(actual.object.equals(object) && actual.values.size() == 1
            && Arrays.equals(value, actual.values.get(attribute))
            && Arrays.equals(tag, actual.tag) && producer.equals(actual.producer)
            && time.equals(actual.time) && actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == receivedOrder && retraction.equals(actual.retraction)
            && actual.regions != null && actual.regions.size() == 1
            && actual.regions.contains(sourceRegion),
         "timestamped regional attribute callback returned incorrect value or metadata");
      check(actual.time.toString() != null && !actual.time.toString().isEmpty(),
         "timestamped regional attribute callback omitted its logical-time text");
      check(actual.transportation != null,
         "timestamped regional attribute callback omitted its transportation type");
      String actualTransportationName = receiver.getTransportationTypeName(actual.transportation);
      check(actualTransportationName != null && !actualTransportationName.isEmpty()
            && (transportationName == null || transportationName.equals(actualTransportationName)),
         "timestamped regional attribute callback returned an invalid or unexpected transport");
   }

   private static LogicalTime addSteps(LogicalTime start, LogicalTimeInterval interval,
         int count) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < count; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void requireRetraction(MessageRetractionReturn result, String label) {
      check(result != null && result.retractionHandleIsValid && result.handle != null,
         label + " returned no valid message retraction handle");
   }

   private static void expectException(JavaTckSupport.CheckedOperation operation,
         String expected, String description) throws Exception {
      JavaTckSupport.expectFailure(operation, expected, description);
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         if (second != null) pump(second, model);
      }
      check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      await(first, second, model, condition, description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         for (int i = 0; i < 8; ++i) ambassador.evokeCallback(0.0);
      } else Thread.yield();
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static void check(boolean condition, String message) {
      JavaTckSupport.check(condition, message);
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions, LogicalTime<?, ?> time, OrderType sentOrder,
            OrderType receivedOrder, MessageRetractionHandle retraction) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
         this.retraction = retraction;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile boolean timeRegulationEnabled;
      private volatile int timeConstrainedEnabledCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5], (LogicalTime<?, ?>) arguments[6],
                     (OrderType) arguments[7], (OrderType) arguments[8],
                     (MessageRetractionHandle) arguments[9]));
                  callbackOrder.add("reflect");
               } else if ("requestRetraction".equals(name) && arguments != null) {
                  retractions.add((MessageRetractionHandle) arguments[0]);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped regional attribute callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
