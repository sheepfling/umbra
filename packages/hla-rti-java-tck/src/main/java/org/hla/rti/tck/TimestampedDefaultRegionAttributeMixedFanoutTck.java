package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
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

/** Tests timestamped default-region update fan-out to immediate and constrained recipients. */
final class TimestampedDefaultRegionAttributeMixedFanoutTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedDefaultRegionAttributeMixedFanoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      RTIambassador immediate = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder constrainedCallbacks = new Recorder();
      Recorder immediateCallbacks = new Recorder();
      String federation = "java-tck-default-region-attribute-mixed-fanout-" + UUID.randomUUID();
      boolean ownerConnected = false, constrainedConnected = false, immediateConnected = false;
      boolean created = false, ownerJoined = false, constrainedJoined = false, immediateJoined = false;
      boolean published = false, constrainedSubscribed = false, immediateSubscribed = false;
      boolean regulating = false, timeConstrained = false;
      ObjectClassHandle ownerClass = null, constrainedClass = null, immediateClass = null;
      AttributeHandle ownerAttribute = null, constrainedAttribute = null, immediateAttribute = null;
      AttributeHandleSet ownerAttributes = null, constrainedAttributes = null, immediateAttributes = null;
      RegionHandle constrainedRegion = null, immediateRegion = null;
      RegionHandleSet constrainedRegions = null, immediateRegions = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel); ownerConnected = true;
         constrained.connect(constrainedCallbacks.proxy(), callbackModel);
         constrainedConnected = true;
         immediate.connect(immediateCallbacks.proxy(), callbackModel); immediateConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-default-region-attribute-owner", federation); ownerJoined = true;
         constrained.joinFederationExecution("java-tck-default-region-attribute-constrained", federation);
         constrainedJoined = true;
         immediate.joinFederationExecution("java-tck-default-region-attribute-immediate", federation);
         immediateJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         constrainedClass = constrained.getObjectClassHandle(objectClassName);
         immediateClass = immediate.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         constrainedAttribute = constrained.getAttributeHandle(constrainedClass, attributeName);
         immediateAttribute = immediate.getAttributeHandle(immediateClass, attributeName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle constrainedFirst = constrained.getDimensionHandle(firstDimensionName);
         DimensionHandle constrainedSecond = constrained.getDimensionHandle(secondDimensionName);
         DimensionHandle immediateFirst = immediate.getDimensionHandle(firstDimensionName);
         DimensionHandle immediateSecond = immediate.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(constrainedClass)
               && ownerClass.equals(immediateClass)
               && ownerAttribute.equals(constrainedAttribute)
               && ownerAttribute.equals(immediateAttribute)
               && ownerFirst.equals(constrainedFirst) && ownerFirst.equals(immediateFirst)
               && ownerSecond.equals(constrainedSecond) && ownerSecond.equals(immediateSecond),
            "adapter DDM FOM returned inconsistent mixed-fanout handles");
         checkDimensions(owner, ownerClass, ownerFirst, ownerSecond);
         checkDimensions(constrained, constrainedClass, constrainedFirst, constrainedSecond);
         checkDimensions(immediate, immediateClass, immediateFirst, immediateSecond);

         ownerAttributes = attributeSet(owner, ownerAttribute);
         constrainedAttributes = attributeSet(constrained, constrainedAttribute);
         immediateAttributes = attributeSet(immediate, immediateAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes); published = true;
         owner.changeDefaultAttributeOrderType(ownerClass, ownerAttributes, OrderType.TIMESTAMP);
         constrainedRegion = createRegion(constrained, constrainedFirst, constrainedSecond);
         immediateRegion = createRegion(immediate, immediateFirst, immediateSecond);
         constrainedRegions = regionSet(constrained, constrainedRegion);
         immediateRegions = regionSet(immediate, immediateRegion);
         constrained.setConveyRegionDesignatorSetsSwitch(true);
         immediate.setConveyRegionDesignatorSetsSwitch(true);
         constrained.subscribeObjectClassAttributesWithRegions(constrainedClass,
            association(constrained, constrainedAttributes, constrainedRegions));
         constrainedSubscribed = true;
         immediate.subscribeObjectClassAttributesWithRegions(immediateClass,
            association(immediate, immediateAttributes, immediateRegions));
         immediateSubscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(owner, constrained, immediate, callbackModel,
            () -> constrainedCallbacks.discoveries.contains(object)
               && immediateCallbacks.discoveries.contains(object), "object discovery");
         LogicalTimeFactory<?, ?> ownerTimeFactory = owner.getTimeFactory();
         JavaTckSupport.check(ownerTimeFactory.getName().equals(constrained.getTimeFactory().getName())
               && ownerTimeFactory.getName().equals(immediate.getTimeFactory().getName()),
            "mixed-fanout members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) ownerTimeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalSteps(epsilon, 5);
         constrained.enableTimeConstrained(); timeConstrained = true;
         await(owner, constrained, immediate, callbackModel,
            () -> constrainedCallbacks.timeConstrainedEnabled,
            "Time Constrained enable callback");
         owner.enableTimeRegulation(lookahead); regulating = true;
         await(owner, constrained, immediate, callbackModel,
            () -> ownerCallbacks.timeRegulationEnabled,
            "Time Regulation enable callback");

         LogicalTime ownerInitial = owner.queryLogicalTime();
         LogicalTime constrainedInitial = constrained.queryLogicalTime();
         LogicalTime messageTime = offset(ownerInitial, epsilon, 6);
         LogicalTime constrainedTarget = offset(constrainedInitial, epsilon, 6);
         LogicalTime ownerTarget = offset(ownerInitial, epsilon, 2);
         JavaTckSupport.check(messageTime.equals(constrainedTarget),
            "owner and constrained recipient computed different message times");
         byte[] value = new byte[] {0x44, 0x52, 0x41, 0x46};
         byte[] tag = new byte[] {0x4d, 0x46, 0x41, 0x4e};
         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(ownerAttribute, value.clone());
         MessageRetractionReturn sent =
            owner.updateAttributeValues(object, values, tag.clone(), messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped default-region update returned no valid retraction handle");
         pump(owner, constrained, immediate, callbackModel);
         JavaTckSupport.check(constrainedCallbacks.reflections.isEmpty(),
            "constrained recipient received the update before requesting an advance");
         await(owner, constrained, immediate, callbackModel,
            () -> immediateCallbacks.reflections.size() == 1,
            "immediate recipient timestamped reflection");
         JavaTckSupport.check(immediateCallbacks.reflections.size() == 1,
            "immediate recipient did not receive exactly one timestamped reflection");
         Reflection delivered = immediateCallbacks.reflections.get(0);
         checkReflection(immediate, delivered, object, immediateAttribute, producer, value, tag,
            messageTime, sent.handle, OrderType.RECEIVE);
         JavaTckSupport.check(immediateCallbacks.grants.isEmpty()
               && immediateCallbacks.callbackOrder.equals(Arrays.asList("reflect")),
            "immediate recipient received an unexpected callback before retraction");

         constrained.timeAdvanceRequest(constrainedTarget);
         JavaTckSupport.check(constrainedCallbacks.grants.isEmpty(),
            "constrained recipient granted before the owner frontier");
         owner.retract(sent.handle);
         JavaTckSupport.check(constrainedCallbacks.reflections.isEmpty()
               && constrainedCallbacks.callbackOrder.isEmpty(),
            "pending constrained copy was delivered before retraction");
         await(owner, constrained, immediate, callbackModel,
            () -> immediateCallbacks.retractions.size() == 1,
            "immediate recipient Request Retraction callback");
         JavaTckSupport.check(sent.handle.equals(immediateCallbacks.retractions.get(0))
               && immediateCallbacks.callbackOrder.equals(Arrays.asList("reflect", "retract")),
            "immediate recipient received incorrect callback ordering or retraction handle");

         owner.timeAdvanceRequest(ownerTarget);
         await(owner, constrained, immediate, callbackModel,
            () -> ownerCallbacks.grants.size() == 1 && constrainedCallbacks.grants.size() == 1,
            "owner and constrained time-advance grants");
         JavaTckSupport.check(ownerTarget.equals(ownerCallbacks.grants.get(0))
               && messageTime.equals(constrainedCallbacks.grants.get(0))
               && constrainedCallbacks.reflections.isEmpty()
               && constrainedCallbacks.retractions.isEmpty()
               && constrainedCallbacks.callbackOrder.equals(Arrays.asList("grant")),
            "constrained recipient received a retracted reflection or incorrect grant");
         JavaTckSupport.expectFailure(() -> owner.retract(sent.handle),
            "MessageCanNoLongerBeRetracted", "retracting a delivered mixed-fanout update twice");

         constrained.disableTimeConstrained(); timeConstrained = false;
         owner.disableTimeRegulation(); regulating = false;
         constrained.unsubscribeObjectClassAttributesWithRegions(constrainedClass,
            association(constrained, constrainedAttributes, constrainedRegions));
         constrainedSubscribed = false;
         immediate.unsubscribeObjectClassAttributesWithRegions(immediateClass,
            association(immediate, immediateAttributes, immediateRegions));
         immediateSubscribed = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); published = false;
         constrained.deleteRegion(constrainedRegion); constrainedRegion = null;
         immediate.deleteRegion(immediateRegion); immediateRegion = null;
      } finally {
         if (timeConstrained && constrainedJoined) try { constrained.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && ownerJoined) try { owner.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (constrainedSubscribed && constrainedJoined) try {
            constrained.unsubscribeObjectClassAttributesWithRegions(constrainedClass,
               association(constrained, constrainedAttributes, constrainedRegions));
         } catch (Exception ignored) { }
         if (immediateSubscribed && immediateJoined) try {
            immediate.unsubscribeObjectClassAttributesWithRegions(immediateClass,
               association(immediate, immediateAttributes, immediateRegions));
         } catch (Exception ignored) { }
         if (published && ownerJoined) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         deleteRegion(constrained, constrainedRegion, constrainedJoined);
         deleteRegion(immediate, immediateRegion, immediateJoined);
         if (immediateJoined) resign(immediate);
         if (constrainedJoined) resign(constrained);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (immediateConnected) disconnect(immediate);
         if (constrainedConnected) disconnect(constrained);
         if (ownerConnected) disconnect(owner);
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
            && ambassador.getDimensionUpperBound(first) >= 6
            && ambassador.getDimensionUpperBound(second) >= 6,
         "adapter FOM object class does not expose two usable DDM dimensions");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(2, 6));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(2, 6));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static hla.rti1516_2025.AttributeSetRegionSetPairList association(
         RTIambassador ambassador, AttributeHandleSet attributes, RegionHandleSet regions)
         throws Exception {
      hla.rti1516_2025.AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new hla.rti1516_2025.AttributeRegionAssociation(attributes, regions));
      return result;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
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

   private static void checkReflection(RTIambassador receiver, Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle attribute, FederateHandle producer,
         byte[] value, byte[] tag, LogicalTime<?, ?> time,
         MessageRetractionHandle retraction, OrderType receivedOrder) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values != null
            && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer)
            && time.equals(reflection.time) && reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == receivedOrder && retraction.equals(reflection.retraction)
            && reflection.regions != null && reflection.regions.isEmpty()
            && reflection.transportation != null,
         "default-region mixed-fanout reflection returned incorrect metadata");
      JavaTckSupport.check(!receiver.getTransportationTypeName(reflection.transportation).isEmpty(),
         "default-region mixed-fanout reflection returned an unresolved transportation handle");
   }

   private static void await(RTIambassador owner, RTIambassador constrained,
         RTIambassador immediate, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(owner, constrained, immediate, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador owner, RTIambassador constrained,
         RTIambassador immediate, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(owner);
         JavaTckSupport.drain(constrained);
         JavaTckSupport.drain(immediate);
      } else {
         Thread.yield();
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region, boolean joined) {
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
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile boolean timeRegulationEnabled, timeConstrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length > 0) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
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
                  callbackOrder.add("reflect");
               } else if ("requestRetraction".equals(name)
                     && arguments != null && arguments.length == 1) {
                  retractions.add((MessageRetractionHandle) arguments[0]);
                  callbackOrder.add("retract");
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK default-region mixed-fanout callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
