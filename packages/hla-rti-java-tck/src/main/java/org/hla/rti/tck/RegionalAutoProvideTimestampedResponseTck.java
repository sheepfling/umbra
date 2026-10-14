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
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
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
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/** Verifies timestamped regional Auto Provide through recipient grant and retraction boundary. */
final class RegionalAutoProvideTimestampedResponseTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] RESPONSE_VALUE = new byte[] {0x52, 0x45, 0x47};
   private static final byte[] RESPONSE_TAG = new byte[] {0x52, 0x41, 0x54, 0x53};
   private RegionalAutoProvideTimestampedResponseTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String dimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      List<String> callbackOrder = new CopyOnWriteArrayList<>();
      List<String> requesterCallbackOrder = new CopyOnWriteArrayList<>();
      AtomicReference<Throwable> callbackFailure = new AtomicReference<>();
      Recorder ownerCallbacks = new Recorder(callbackOrder, requesterCallbackOrder, callbackFailure, false);
      Recorder requesterCallbacks = new Recorder(callbackOrder, requesterCallbackOrder, callbackFailure, true);
      String federation = "java-tck-regional-auto-provide-tso-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false, published = false, subscribed = false;
      boolean ownerRegulating = false, requesterConstrained = false;
      boolean ownerRegionCreated = false, requesterRegionCreated = false, associated = false;
      ObjectClassHandle ownerClass = null, requesterClass = null;
      AttributeHandle ownerAttribute = null, requesterAttribute = null;
      AttributeHandleSet ownerAttributes = null, requesterAttributes = null;
      AttributeSetRegionSetPairList ownerPair = null, requesterPair = null;
      RegionHandle ownerRegion = null, requesterRegion = null;
      ObjectInstanceHandle object = null;
      try {
         ownerCallbacks.configureOwner(owner);
         owner.connect(ownerCallbacks.proxy(), callbackModel); ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel); requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "regional-auto-provide-tso-owner", "owner", federation); ownerJoined = true;
         requester.joinFederationExecution("regional-auto-provide-tso-requester", "requester",
            federation); requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         DimensionHandle ownerDimension = owner.getDimensionHandle(dimensionName);
         DimensionHandle requesterDimension = requester.getDimensionHandle(dimensionName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute)
               && ownerDimension.equals(requesterDimension),
            "adapter timestamped Auto Provide FOM returned inconsistent handles");
         JavaTckSupport.check(owner.getAutoProvideSwitch() && requester.getAutoProvideSwitch(),
            "adapter FOM did not enable Auto Provide for both members");
         checkDimension(owner, ownerClass, ownerDimension);
         checkDimension(requester, requesterClass, requesterDimension);

         LogicalTimeFactory<?, ?> ownerTimeFactory = owner.getTimeFactory();
         LogicalTimeFactory<?, ?> requesterTimeFactory = requester.getTimeFactory();
         JavaTckSupport.check(ownerTimeFactory != null && requesterTimeFactory != null
               && timeImplementation.equals(ownerTimeFactory.getName())
               && timeImplementation.equals(requesterTimeFactory.getName()),
            "federates selected a different logical-time factory than the adapter configured");
         LogicalTime ownerInitial = (LogicalTime) ownerTimeFactory.makeInitial();
         LogicalTimeInterval ownerEpsilon = (LogicalTimeInterval) ownerTimeFactory.makeEpsilon();
         LogicalTime requesterInitial = (LogicalTime) requesterTimeFactory.makeInitial();
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterTimeFactory.makeEpsilon();
         LogicalTime responseTime = offset(ownerInitial, ownerEpsilon, 2);
         LogicalTime requesterTarget = offset(requesterInitial, requesterEpsilon, 2);
         JavaTckSupport.check(ownerInitial.equals(requesterInitial)
               && responseTime.equals(requesterTarget),
            "regional Auto Provide members did not calculate the same target time");

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         AttributeHandleValueMap responseValues =
            owner.getAttributeHandleValueMapFactory().create(1);
         responseValues.put(ownerAttribute, RESPONSE_VALUE.clone());
         ownerCallbacks.configureResponse(ownerAttribute, responseValues, responseTime);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes); published = true;
         owner.changeDefaultAttributeOrderType(ownerClass, ownerAttributes, OrderType.TIMESTAMP);
         ownerRegion = createRegion(owner, ownerDimension, 0, 2); ownerRegionCreated = true;
         requesterRegion = createRegion(requester, requesterDimension, 1, 3);
         requesterRegionCreated = true;
         ownerPair = association(owner, ownerAttributes, ownerRegion);
         requesterPair = association(requester, requesterAttributes, requesterRegion);
         requester.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(requester.getConveyRegionDesignatorSetsSwitch(),
            "timestamped Auto Provide requester did not enable region-designator callbacks");

         requester.enableTimeConstrained(); requesterConstrained = true;
         owner.enableTimeRegulation(ownerEpsilon); ownerRegulating = true;
         await(owner, requester, callbackModel,
            () -> ownerCallbacks.timeRegulationEnabled.get()
               && requesterCallbacks.timeConstrainedEnabled.get(), "time-role enable callbacks");

         object = owner.registerObjectInstanceWithRegions(ownerClass, ownerPair); associated = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "timestamped regional Auto Provide registration returned an invalid object handle");
         requester.subscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         subscribed = true;
         ObjectInstanceHandle registeredObject = object;
         await(owner, requester, callbackModel,
            () -> requesterCallbacks.discoveries.size() == 1 && ownerCallbacks.provided.size() == 1,
            "timestamped regional Auto Provide discovery and provider request");
         if (callbackFailure.get() != null) {
            throw new AssertionError("timestamped regional Auto Provide callback failed",
               callbackFailure.get());
         }
         JavaTckSupport.check(requesterCallbacks.reflections.isEmpty(),
            "timestamped Auto Provide response reflected before the requester advanced");
         JavaTckSupport.check(callbackOrder.equals(Arrays.asList("discover", "provide")),
            "timestamped Auto Provide callback order was not discovery then provide");
         Discovery discovery = requesterCallbacks.discoveries.get(0);
         JavaTckSupport.check(registeredObject.equals(discovery.object)
               && requesterClass.equals(discovery.objectClass)
               && producer.equals(discovery.producer),
            "timestamped Auto Provide discovery returned incorrect object metadata");
         ProvidedUpdate provided = ownerCallbacks.provided.get(0);
         JavaTckSupport.check(registeredObject.equals(provided.object)
               && provided.attributes.size() == 1 && provided.attributes.contains(ownerAttribute)
               && provided.tag.length == 0,
            "timestamped Auto Provide request returned incorrect object, attributes, or tag");
         MessageRetractionReturn retraction = ownerCallbacks.retraction.get();
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Auto Provide update returned no valid retraction handle");

         requester.timeAdvanceRequest(requesterTarget);
         owner.timeAdvanceRequest(responseTime);
         await(owner, requester, callbackModel,
            () -> requesterCallbacks.reflections.size() == 1
               && requesterCallbacks.grants.size() == 1 && ownerCallbacks.grants.size() == 1,
            "timestamped Auto Provide reflection and time grants");
         JavaTckSupport.check(callbackOrder.equals(Arrays.asList("discover", "provide", "reflect"))
               && requesterCallbackOrder.equals(Arrays.asList("reflect", "grant")),
            "timestamped Auto Provide callback order did not deliver reflection before grant");
         Reflection reflection = requesterCallbacks.reflections.get(0);
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(registeredObject.equals(reflection.object)
               && reflection.values.size() == 1
               && Arrays.equals(RESPONSE_VALUE, reflection.values.get(requesterAttribute))
               && Arrays.equals(RESPONSE_TAG, reflection.tag)
               && reliable.equals(reflection.transportation)
               && producer.equals(reflection.producer)
               && reflection.regions != null && reflection.regions.size() == 1
               && reflection.regions.contains(ownerRegion)
               && responseTime.equals(reflection.time)
               && reflection.sentOrder == OrderType.TIMESTAMP
               && reflection.receivedOrder == OrderType.TIMESTAMP
               && reflection.retraction != null,
            "timestamped regional Auto Provide reflection lost its payload or time metadata");
         JavaTckSupport.check(responseTime.equals(requesterCallbacks.grants.get(0))
               && responseTime.equals(ownerCallbacks.grants.get(0))
               && responseTime.equals(requester.queryLogicalTime()),
            "timestamped regional Auto Provide returned an incorrect grant time");
         JavaTckSupport.expectFailure(() -> owner.retract(retraction.handle),
            "MessageCanNoLongerBeRetracted", "retracting delivered timestamped Auto Provide response");

         requester.disableTimeConstrained(); requesterConstrained = false;
         owner.disableTimeRegulation(); ownerRegulating = false;
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
         if (requesterJoined && requesterConstrained) try { requester.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (ownerJoined && ownerRegulating) try { owner.disableTimeRegulation(); }
            catch (Exception ignored) { }
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
         "adapter Auto Provide object class must expose its configured dimension through range 3");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
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
      private LogicalTime time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private final List<String> callbackOrder;
      private final List<String> requesterCallbackOrder;
      private final boolean receiver;
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<LogicalTime> grants = new CopyOnWriteArrayList<>();
      private final AtomicBoolean timeConstrainedEnabled = new AtomicBoolean();
      private final AtomicBoolean timeRegulationEnabled = new AtomicBoolean();
      private final AtomicReference<RTIambassador> owner = new AtomicReference<>();
      private final AtomicReference<AttributeHandle> expectedAttribute = new AtomicReference<>();
      private final AtomicReference<AttributeHandleValueMap> responseValues = new AtomicReference<>();
      private final AtomicReference<LogicalTime> responseTime = new AtomicReference<>();
      private final AtomicReference<MessageRetractionReturn> retraction = new AtomicReference<>();
      private final AtomicReference<Throwable> callbackFailure;

      private Recorder(List<String> callbackOrder, List<String> requesterCallbackOrder,
            AtomicReference<Throwable> callbackFailure, boolean receiver) {
         this.callbackOrder = callbackOrder;
         this.requesterCallbackOrder = requesterCallbackOrder;
         this.callbackFailure = callbackFailure;
         this.receiver = receiver;
      }

      private void configureOwner(RTIambassador ambassador) {
         owner.set(ambassador);
      }

      private void configureResponse(AttributeHandle attribute, AttributeHandleValueMap values,
            LogicalTime time) {
         expectedAttribute.set(attribute); responseValues.set(values); responseTime.set(time);
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
                     AttributeHandle attribute = expectedAttribute.get();
                     if (update.attributes.size() != 1 || !update.attributes.contains(attribute)) {
                        throw new IllegalStateException("timestamped Auto Provide requested an unexpected attribute");
                     }
                     retraction.set(owner.get().updateAttributeValues(update.object,
                        responseValues.get(), RESPONSE_TAG.clone(), responseTime.get()));
                  } catch (Throwable failure) {
                     callbackFailure.compareAndSet(null, failure);
                  }
               } else if ("reflectAttributeValues".equals(name) && args != null
                     && args.length == 10) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) args[0];
                  reflection.values = ((AttributeHandleValueMap) args[1]).clone();
                  reflection.tag = ((byte[]) args[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) args[3];
                  reflection.producer = (FederateHandle) args[4];
                  reflection.regions = (RegionHandleSet) args[5];
                  reflection.time = (LogicalTime) args[6];
                  reflection.sentOrder = (OrderType) args[7];
                  reflection.receivedOrder = (OrderType) args[8];
                  reflection.retraction = (MessageRetractionHandle) args[9];
                  reflections.add(reflection); callbackOrder.add("reflect");
                  requesterCallbackOrder.add("reflect");
               } else if ("timeConstrainedEnabled".equals(name) && args != null) {
                  timeConstrainedEnabled.set(true);
               } else if ("timeRegulationEnabled".equals(name) && args != null) {
                  timeRegulationEnabled.set(true);
               } else if ("timeAdvanceGrant".equals(name) && args != null) {
                  grants.add((LogicalTime) args[0]);
                  if (receiver) requesterCallbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped regional Auto Provide callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
