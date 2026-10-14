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

/** Preserves queued timestamped regional delivery after its producer resigns. */
final class TimestampedRegionalAttributeSourceResignationTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedRegionalAttributeSourceResignationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador producer = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder producerCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      Recorder clockCallbacks = new Recorder();
      String federation = "java-tck-tso-regional-source-resignation-" + UUID.randomUUID();
      boolean producerConnected = false, receiverConnected = false, clockConnected = false;
      boolean created = false, producerJoined = false, receiverJoined = false, clockJoined = false;
      boolean published = false, subscribed = false, constrained = false, clockRegulating = false;
      ObjectClassHandle producerClass = null, receiverClass = null;
      AttributeHandle producerAttribute = null, receiverAttribute = null;
      AttributeHandleSet producerAttributes = null, receiverAttributes = null;
      AttributeSetRegionSetPairList receiverPair = null;
      RegionHandle receiverRegion = null;
      try {
         producer.connect(producerCallbacks.proxy(), callbackModel);
         producerConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         clock.connect(clockCallbacks.proxy(), callbackModel);
         clockConnected = true;
         producer.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producingFederate = producer.joinFederationExecution(
            "java-tck-regional-resignation-producer", federation);
         producerJoined = true;
         receiver.joinFederationExecution("java-tck-regional-resignation-receiver", federation);
         receiverJoined = true;
         clock.joinFederationExecution("java-tck-regional-resignation-clock", federation);
         clockJoined = true;

         producerClass = producer.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         producerAttribute = producer.getAttributeHandle(producerClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         DimensionHandle producerFirst = producer.getDimensionHandle(firstDimensionName);
         DimensionHandle producerSecond = producer.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(producerClass.equals(receiverClass)
               && producerAttribute.equals(receiverAttribute)
               && producerFirst.equals(receiverFirst) && producerSecond.equals(receiverSecond),
            "adapter DDM FOM returned inconsistent class, attribute, or dimension handles");
         checkDimensions(producer, producerClass, producerFirst, producerSecond);
         checkDimensions(receiver, receiverClass, receiverFirst, receiverSecond);

         producerAttributes = JavaTckSupport.attributeSet(producer, producerAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         producer.publishObjectClassAttributes(producerClass, producerAttributes);
         published = true;
         producer.changeDefaultAttributeOrderType(
            producerClass, producerAttributes, OrderType.TIMESTAMP);
         RegionHandle producerRegion = createRegion(
            producer, producerFirst, producerSecond, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6);
         AttributeSetRegionSetPairList producerPair = association(
            producer, producerAttributes, producerRegion);
         receiverPair = association(receiver, receiverAttributes, receiverRegion);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = true;
         ObjectInstanceHandle object = producer.registerObjectInstanceWithRegions(
            producerClass, producerPair);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional source-resignation registration returned an invalid object handle");
         await(producer, receiver, clock, callbackModel,
            () -> receiverCallbacks.discoveries.contains(object), "regional object discovery");

         LogicalTimeFactory<?, ?> producerFactory = producer.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> clockFactory = clock.getTimeFactory();
         JavaTckSupport.check(producerFactory.getName().equals(receiverFactory.getName())
               && producerFactory.getName().equals(clockFactory.getName()),
            "regional resignation members selected different logical-time factories");
         LogicalTimeInterval producerEpsilon = (LogicalTimeInterval) producerFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon = (LogicalTimeInterval) receiverFactory.makeEpsilon();
         LogicalTimeInterval clockEpsilon = (LogicalTimeInterval) clockFactory.makeEpsilon();
         LogicalTime producerInitial = producer.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime clockInitial = clock.queryLogicalTime();
         JavaTckSupport.check(producerInitial.equals(receiverInitial)
               && producerInitial.equals(clockInitial),
            "regional resignation members began at different logical times");
         LogicalTimeInterval producerLookahead = intervalSteps(producerEpsilon, 5);
         LogicalTimeInterval clockLookahead = intervalSteps(clockEpsilon, 5);

         receiver.enableTimeConstrained();
         constrained = true;
         producer.enableTimeRegulation(producerLookahead);
         clock.enableTimeRegulation(clockLookahead);
         clockRegulating = true;
         await(producer, receiver, clock, callbackModel,
            () -> receiverCallbacks.timeConstrainedEnabled
               && producerCallbacks.timeRegulationEnabled && clockCallbacks.timeRegulationEnabled,
            "regional resignation time-role callbacks");

         LogicalTime messageTime = offset(producerInitial, producerEpsilon, 7);
         LogicalTime receiverTarget = offset(receiverInitial, receiverEpsilon, 7);
         LogicalTime clockTarget = offset(clockInitial, clockEpsilon, 2);
         byte[] value = new byte[] {(byte) 0xA9, 0x72, 0x31};
         byte[] tag = new byte[] {0x52, 0x45, 0x47, 0x2D, 0x52};
         AttributeHandleValueMap values = producer.getAttributeHandleValueMapFactory().create(1);
         values.put(producerAttribute, value.clone());
         MessageRetractionReturn update = producer.updateAttributeValues(
            object, values, tag.clone(), messageTime);
         JavaTckSupport.check(update != null && update.retractionHandleIsValid
               && update.handle != null,
            "regional source-resignation update returned no valid retraction handle");
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty(),
            "regional source-resignation update reflected before the receiver frontier");

         receiver.timeAdvanceRequest(receiverTarget);
         JavaTckSupport.check(receiverCallbacks.grants.isEmpty(),
            "regional source-resignation receiver advanced before an independent frontier");
         producer.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         producerJoined = false;
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty()
               && receiverCallbacks.grants.isEmpty(),
            "producer resignation released the queued regional update too early");
         JavaTckSupport.expectFailure(() -> producer.retract(update.handle),
            "FederateNotExecutionMember", "retracting after regional producer resignation");

         clock.timeAdvanceRequest(clockTarget);
         await(producer, receiver, clock, callbackModel,
            () -> clockCallbacks.grants.size() >= 1
               && receiverCallbacks.reflections.size() >= 1 && receiverCallbacks.grants.size() >= 1,
            "regional update and recipient grant after producer resignation");
         JavaTckSupport.check(receiverCallbacks.reflections.size() == 1,
            "regional source resignation delivered a duplicate reflection");
         Reflection reflection = receiverCallbacks.reflections.get(0);
         assertReflection(receiver, reflection, object, receiverAttribute, producingFederate,
            value, tag, messageTime, update.handle, producerRegion);
         JavaTckSupport.check(receiverTarget.equals(receiverCallbacks.grants.get(0))
               && clockTarget.equals(clockCallbacks.grants.get(0))
               && receiverTarget.equals(receiver.queryLogicalTime()),
            "regional source-resignation advances returned incorrect grant times");
         JavaTckSupport.check(receiverCallbacks.callbackOrder.equals(
               Arrays.asList("reflectAttributeValues", "timeAdvanceGrant")),
            "regional source-resignation reflection was delivered after its grant");

         receiver.disableTimeConstrained();
         constrained = false;
         clock.disableTimeRegulation();
         clockRegulating = false;
         receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         subscribed = false;
         receiver.deleteRegion(receiverRegion);
         receiverRegion = null;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         clock.resignFederationExecution(ResignAction.NO_ACTION);
         clockJoined = false;
         receiver.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (clockRegulating && clockJoined) try { clock.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined && receiverPair != null) try {
            receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         } catch (Exception ignored) { }
         if (published && producerJoined) try {
            producer.unpublishObjectClassAttributes(producerClass, producerAttributes);
         } catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (clockJoined) try { clock.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (producerJoined) try {
            producer.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         } catch (Exception ignored) { }
         if (created) try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (producerConnected) try { producer.disconnect(); } catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (clockConnected) try { clock.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      java.util.Set<DimensionHandle> dimensions =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(dimensions.contains(first) && dimensions.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 6
            && ambassador.getDimensionUpperBound(second) >= 6,
         "adapter FOM does not expose the required regional source-resignation dimensions");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
      return region;
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

   private static LogicalTimeInterval intervalSteps(LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTimeInterval result = interval;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(interval);
      return result;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void assertReflection(RTIambassador receiver, Reflection reflection,
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
         "regional source-resignation reflection lost value or timestamped metadata");
      TransportationTypeHandle transportation = reflection.transportation;
      JavaTckSupport.check(transportation != null
            && !receiver.getTransportationTypeName(transportation).isEmpty(),
         "regional source-resignation reflection returned an unresolvable transportation type");
   }

   private static void await(RTIambassador first, RTIambassador second, RTIambassador third,
         CallbackModel model, java.util.function.BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         pump(second, model);
         pump(third, model);
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
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional source-resignation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
