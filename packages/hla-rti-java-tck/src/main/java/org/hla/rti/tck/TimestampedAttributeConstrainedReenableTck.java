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
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
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

/** Preserves queued timestamped attribute delivery across Time Constrained re-enable. */
final class TimestampedAttributeConstrainedReenableTck {
   private TimestampedAttributeConstrainedReenableTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      runInternal(factory, fom, timeImplementation, objectClassName, attributeName,
         null, null, callbackModel, false);
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
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-attribute-constrained-reenable-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean created = false;
      boolean regulating = false;
      boolean constrained = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet receiverAttributes = null;
      AttributeSetRegionSetPairList receiverPair = null;
      RegionHandle receiverRegion = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-attribute-constrained-reenable-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(
            "java-tck-attribute-constrained-reenable-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         if (defaultRegion) {
            DimensionHandle first = receiver.getDimensionHandle(firstDimensionName);
            DimensionHandle second = receiver.getDimensionHandle(secondDimensionName);
            java.util.Set<DimensionHandle> available =
               receiver.getAvailableDimensionsForObjectClass(receiverClass);
            JavaTckSupport.check(available.contains(first) && available.contains(second)
                  && receiver.getDimensionUpperBound(first) >= 1
                  && receiver.getDimensionUpperBound(second) >= 1,
               "adapter FOM object class does not expose the configured DDM dimensions");
            DimensionHandleSet dimensions = receiver.getDimensionHandleSetFactory().create();
            dimensions.add(first);
            dimensions.add(second);
            receiverRegion = receiver.createRegion(dimensions);
            receiver.setRangeBounds(receiverRegion, first,
               new hla.rti1516_2025.RangeBounds(0, 1));
            receiver.setRangeBounds(receiverRegion, second,
               new hla.rti1516_2025.RangeBounds(0, 1));
            RegionHandleSet regions = receiver.getRegionHandleSetFactory().create();
            regions.add(receiverRegion);
            receiver.commitRegionModifications(regions);
            receiverPair = receiver.getAttributeSetRegionSetPairListFactory().create(1);
            receiverPair.add(new AttributeRegionAssociation(receiverAttributes, regions));
            receiver.setConveyRegionDesignatorSetsSwitch(true);
            JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
               "receiver did not enable region designator delivery");
            receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         } else {
            receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         }
         subscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(object.equals(receiverRecorder.discoveredObject),
            "receiver did not discover the registered object");
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "timestamped attribute members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         constrained = true;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabledCount == 1,
            "receiver did not report its initial Time Constrained enable callback");
         publisher.enableTimeRegulation(epsilon);
         regulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabled,
            "publisher did not report its Time Regulation enable callback");

         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiver.queryLogicalTime()),
            "publisher and receiver began at different logical times");
         LogicalTime updateTime = offset(initial, epsilon, 5);
         LogicalTime publisherTarget = offset(initial, epsilon, 4);
         byte[] value = new byte[] {0x41, 0x54, 0x54, 0x52};
         byte[] tag = new byte[] {0x52, 0x45, 0x45, 0x4e, 0x2d, 0x41};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value);
         MessageRetractionReturn sent = publisher.updateAttributeValues(
            object, values, tag, updateTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped Update Attribute Values returned no valid retraction handle");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "timestamped reflection arrived before its constrained grant");

         receiver.disableTimeConstrained();
         constrained = false;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "queued reflection was delivered while Time Constrained was disabled");
         receiver.enableTimeConstrained();
         constrained = true;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabledCount == 2,
            "receiver did not report its second Time Constrained enable callback");
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "queued reflection was delivered during Time Constrained re-enable");

         receiverRecorder.callbackOrder.clear();
         publisherRecorder.callbackOrder.clear();
         receiver.timeAdvanceRequest(updateTime);
         publisher.timeAdvanceRequest(publisherTarget);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(receiverRecorder.reflections.size() == 1
               && receiverRecorder.timeConstrainedEnabledCount == 2,
            "re-enabled receiver did not get exactly one timestamped reflection");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("reflectAttributeValues", "timeAdvanceGrant")),
            "queued reflection was delivered after its grant");
         JavaTckSupport.check(updateTime.equals(receiverRecorder.timeAdvanceGrant)
               && publisherTarget.equals(publisherRecorder.timeAdvanceGrant)
               && updateTime.equals(receiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "time-advance grants or logical-time queries returned the wrong times");
         checkReflection(receiver, receiverRecorder.reflections.get(0), object,
            receiverAttribute, value, tag, producer, updateTime, sent.handle, defaultRegion);
         JavaTckSupport.expectFailure(() -> publisher.retract(sent.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting the attribute update after its delivery grant");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try {
               if (defaultRegion) {
                  receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
               } else {
                  receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
               }
            }
            catch (Exception ignored) { }
         }
         if (receiverRegion != null && receiverJoined) {
            try { receiver.deleteRegion(receiverRegion); } catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void checkReflection(RTIambassador receiver, Reflection reflection,
         ObjectInstanceHandle expectedObject, AttributeHandle expectedAttribute,
         byte[] expectedValue, byte[] expectedTag, FederateHandle expectedProducer,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         boolean defaultRegion)
         throws Exception {
      JavaTckSupport.check(expectedObject.equals(reflection.object)
            && reflection.values != null && reflection.values.size() == 1
            && Arrays.equals(expectedValue, reflection.values.get(expectedAttribute)),
         "timestamped re-enabled reflection returned the wrong object or value");
      JavaTckSupport.check(Arrays.equals(expectedTag, reflection.tag)
            && expectedTime.equals(reflection.time),
         "timestamped re-enabled reflection did not preserve tag or timestamp");
      JavaTckSupport.check(expectedProducer.equals(reflection.producer)
            && reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP,
         "timestamped re-enabled reflection returned the wrong producer or order");
      JavaTckSupport.check(reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty(),
         "timestamped re-enabled reflection returned an unresolvable transportation handle");
      JavaTckSupport.check(defaultRegion
            ? reflection.regions != null && reflection.regions.isEmpty()
            : reflection.regions == null || reflection.regions.isEmpty(),
         defaultRegion
            ? "default-region timestamped update did not report supplied-empty source metadata"
            : "ordinary timestamped update unexpectedly included region metadata");
      JavaTckSupport.check(expectedRetraction.equals(reflection.retraction),
         "timestamped re-enabled reflection returned the wrong retraction handle");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private RegionHandleSet regions;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile int timeConstrainedEnabledCount;
      private volatile ObjectInstanceHandle discoveredObject;
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabledCount++;
               } else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
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
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute re-enable callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
