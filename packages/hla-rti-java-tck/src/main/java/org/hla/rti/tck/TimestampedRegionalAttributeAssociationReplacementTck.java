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

/** Proves queued timestamped updates retain their source-region association snapshot. */
final class TimestampedRegionalAttributeAssociationReplacementTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedRegionalAttributeAssociationReplacementTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-tso-regional-association-replacement-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false, published = false;
      boolean subscribed = false, constrained = false, regulating = false;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandle publisherAttribute = null, receiverAttribute = null;
      AttributeHandleSet publisherAttributes = null, receiverAttributes = null;
      AttributeSetRegionSetPairList sourcePairA = null, sourcePairB = null, receiverPair = null;
      RegionHandle sourceRegionA = null, sourceRegionB = null, receiverRegion = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-regional-association-producer", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-regional-association-receiver", federation);
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
         sourceRegionA = createRegion(publisher, publisherFirst, publisherSecond);
         sourceRegionB = createRegion(publisher, publisherFirst, publisherSecond);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond);
         sourcePairA = association(publisher, publisherAttributes, sourceRegionA);
         sourcePairB = association(publisher, publisherAttributes, sourceRegionB);
         receiverPair = association(receiver, receiverAttributes, receiverRegion);
         ObjectInstanceHandle object = publisher.registerObjectInstanceWithRegions(
            publisherClass, sourcePairA);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional association replacement returned an invalid object handle");
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = true;
         await(publisher, receiver, callbackModel,
            () -> receiverCallbacks.discoveries.contains(object), "regional object discovery");
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "receiver did not enable region-designator callbacks");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "association-replacement members selected different logical-time factories");
         LogicalTimeInterval publisherEpsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon = (LogicalTimeInterval) receiverFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         constrained = true;
         publisher.enableTimeRegulation(publisherEpsilon);
         regulating = true;
         await(publisher, receiver, callbackModel,
            () -> publisherCallbacks.timeRegulationEnabled
               && receiverCallbacks.timeConstrainedEnabled,
            "association-replacement time-role callbacks");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "association-replacement members began at different logical times");
         LogicalTime firstTime = offset(publisherInitial, publisherEpsilon, 6);
         LogicalTime receiverFirstTime = offset(receiverInitial, receiverEpsilon, 6);
         LogicalTime firstPublisherTarget = offset(publisherInitial, publisherEpsilon, 2);
         byte[] firstValue = new byte[] {0x53, 0x52, 0x31};
         byte[] firstTag = new byte[] {0x52, 0x45, 0x50, 0x31};
         MessageRetractionReturn first = update(
            publisher, object, publisherAttribute, firstValue, firstTag, firstTime);
         requireRetraction(first, "first timestamped update");
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty(),
            "first timestamped regional update arrived before the receiver grant");

         publisher.unassociateRegionsForUpdates(object, sourcePairA);
         publisher.associateRegionsForUpdates(object, sourcePairB);
         receiver.timeAdvanceRequest(receiverFirstTime);
         publisher.timeAdvanceRequest(firstPublisherTarget);
         await(publisher, receiver, callbackModel,
            () -> publisherCallbacks.grants.size() >= 1 && receiverCallbacks.grants.size() >= 1,
            "first source-replacement grants");
         pump(receiver, callbackModel);
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty()
               && receiverCallbacks.retractions.isEmpty(),
            "queued update A was retargeted to replacement source region B");
         receiverCallbacks.callbackOrder.clear();
         publisherCallbacks.callbackOrder.clear();

         LogicalTime secondTime = offset(firstTime, publisherEpsilon, 1);
         LogicalTime receiverSecondTime = offset(receiverFirstTime, receiverEpsilon, 1);
         LogicalTime secondPublisherTarget = offset(firstPublisherTarget, publisherEpsilon, 1);
         byte[] secondValue = new byte[] {0x53, 0x52, 0x32};
         byte[] secondTag = new byte[] {0x52, 0x45, 0x50, 0x32};
         MessageRetractionReturn second = update(
            publisher, object, publisherAttribute, secondValue, secondTag, secondTime);
         requireRetraction(second, "second timestamped update");
         receiver.timeAdvanceRequest(receiverSecondTime);
         publisher.timeAdvanceRequest(secondPublisherTarget);
         await(publisher, receiver, callbackModel,
            () -> publisherCallbacks.grants.size() >= 2 && receiverCallbacks.grants.size() >= 2
               && receiverCallbacks.reflections.size() >= 1,
            "second replacement-region delivery");
         JavaTckSupport.check(receiverCallbacks.reflections.size() == 1,
            "association replacement returned the wrong reflection count");
         assertReflection(receiver, receiverCallbacks.reflections.get(0), object,
            receiverAttribute, producer, secondValue, secondTag, secondTime,
            second.handle, sourceRegionB);
         JavaTckSupport.check(receiverCallbacks.callbackOrder.equals(
               Arrays.asList("reflect", "grant")),
            "replacement-region reflection was delivered after its time grant");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = false;
         publisher.unassociateRegionsForUpdates(object, sourcePairB);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         receiver.deleteRegion(receiverRegion); receiverRegion = null;
         publisher.deleteRegion(sourceRegionB); sourceRegionB = null;
         publisher.deleteRegion(sourceRegionA); sourceRegionA = null;
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
         deleteRegion(publisher, sourceRegionB, publisherJoined);
         deleteRegion(publisher, sourceRegionA, publisherJoined);
         if (receiverJoined) resign(receiver);
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
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
            && ambassador.getDimensionUpperBound(first) >= 3
            && ambassador.getDimensionUpperBound(second) >= 3,
         "adapter FOM object class does not expose the required dimensional attribute");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle result = ambassador.createRegion(dimensions);
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(result);
      ambassador.setRangeBounds(result, first, new hla.rti1516_2025.RangeBounds(1, 3));
      ambassador.setRangeBounds(result, second, new hla.rti1516_2025.RangeBounds(1, 3));
      ambassador.commitRegionModifications(regions);
      return result;
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

   private static MessageRetractionReturn update(RTIambassador ambassador,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value, byte[] tag,
         LogicalTime<?, ?> time) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      return ambassador.updateAttributeValues(object, values, tag.clone(), time);
   }

   private static void assertReflection(RTIambassador ambassador, Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle attribute, FederateHandle producer,
         byte[] value, byte[] tag, LogicalTime<?, ?> time,
         MessageRetractionHandle retraction, RegionHandle expectedRegion) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer)
            && time.equals(reflection.time) && reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(reflection.retraction) && reflection.regions != null
            && reflection.regions.size() == 1 && reflection.regions.contains(expectedRegion),
         "replacement update did not preserve value, time, order, retraction, and source region");
      TransportationTypeHandle transportation = reflection.transportation;
      JavaTckSupport.check(transportation != null
            && !ambassador.getTransportationTypeName(transportation).isEmpty(),
         "replacement update returned an invalid transportation type");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
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

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static void requireRetraction(MessageRetractionReturn result, String description) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid && result.handle != null,
         description + " returned an invalid retraction handle");
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
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile boolean timeRegulationEnabled, timeConstrainedEnabled;

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
                  timeConstrainedEnabled = true;
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
               } else if ("requestRetraction".equals(name) && arguments != null) {
                  retractions.add((MessageRetractionHandle) arguments[0]);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional association callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
