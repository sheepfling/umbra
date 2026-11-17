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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Regional class requests across an inheritance hierarchy and their timestamped response. */
final class RegionalClassRequestUpdateAcrossHierarchyTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final String BASE_CLASS = "HLAobjectRoot.TckKnownClassObject";
   private static final String DERIVED_CLASS =
      "HLAobjectRoot.TckKnownClassObject.TckKnownClassDerivedObject";

   private RegionalClassRequestUpdateAcrossHierarchyTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String firstDimensionName, String secondDimensionName, CallbackModel model)
         throws Exception {
      hierarchy(factory, fom, firstDimensionName, secondDimensionName, model);
      timestampedResponse(factory, fom, timeImplementation, firstDimensionName,
         secondDimensionName, model);
   }

   private static void hierarchy(RtiFactory factory, String fom,
         String firstDimensionName, String secondDimensionName, CallbackModel model)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-regional-class-request-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false;
      boolean ownerBasePublished = false, ownerDerivedPublished = false;
      boolean requesterPublished = false, subscribed = false;
      ObjectClassHandle ownerBase = null, ownerDerived = null, requesterBase = null;
      AttributeHandle ownerBaseAttribute = null, ownerDerivedAttribute = null;
      AttributeHandle requesterBaseAttribute = null;
      AttributeHandleSet ownerBaseAttributes = null, ownerDerivedAttributes = null;
      AttributeHandleSet requesterAttributes = null;
      AttributeSetRegionSetPairList sourcePair = null, derivedPair = null;
      AttributeSetRegionSetPairList excludedPair = null, requestPair = null;
      RegionHandle sourceRegion = null, excludedRegion = null, requestRegion = null;
      boolean sourceRegionCreated = false, excludedRegionCreated = false;
      boolean requestRegionCreated = false;
      List<ObjectInstanceHandle> ownerObjects = new ArrayList<>();
      ObjectInstanceHandle requesterObject = null;
      try {
         owner.connect(ownerCallbacks.proxy(), model);
         ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), model);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("regional-class-request-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("regional-class-request-requester", federation);
         requesterJoined = true;

         ownerBase = owner.getObjectClassHandle(BASE_CLASS);
         ownerDerived = owner.getObjectClassHandle(DERIVED_CLASS);
         requesterBase = requester.getObjectClassHandle(BASE_CLASS);
         ownerBaseAttribute = owner.getAttributeHandle(ownerBase, "Name");
         ownerDerivedAttribute = owner.getAttributeHandle(ownerDerived, "Name");
         requesterBaseAttribute = requester.getAttributeHandle(requesterBase, "Name");
         JavaTckSupport.check(ownerBase != null && ownerDerived != null && requesterBase != null
               && ownerBaseAttribute != null && ownerDerivedAttribute != null
               && requesterBaseAttribute != null,
            "regional class-request hierarchy lookup returned a missing handle");
         Dimensions ownerDimensions = dimensions(owner, ownerBase, firstDimensionName,
            secondDimensionName, "owner base class");
         dimensions(owner, ownerDerived, firstDimensionName, secondDimensionName,
            "owner derived class");
         Dimensions requesterDimensions = dimensions(requester, requesterBase,
            firstDimensionName, secondDimensionName, "requester base class");

         ownerBaseAttributes = JavaTckSupport.attributeSet(owner, ownerBaseAttribute);
         ownerDerivedAttributes = JavaTckSupport.attributeSet(owner, ownerDerivedAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterBaseAttribute);
         owner.publishObjectClassAttributes(ownerBase, ownerBaseAttributes);
         ownerBasePublished = true;
         owner.publishObjectClassAttributes(ownerDerived, ownerDerivedAttributes);
         ownerDerivedPublished = true;
         requester.publishObjectClassAttributes(requesterBase, requesterAttributes);
         requesterPublished = true;

         sourceRegion = createRegion(owner, ownerDimensions, 1, 5);
         sourceRegionCreated = true;
         requestRegion = createRegion(requester, requesterDimensions, 2, 6);
         requestRegionCreated = true;
         excludedRegion = createRegion(owner, ownerDimensions, 7, 9);
         excludedRegionCreated = true;
         sourcePair = association(owner, ownerBaseAttributes, sourceRegion);
         derivedPair = association(owner, ownerDerivedAttributes, sourceRegion);
         excludedPair = association(owner, ownerBaseAttributes, excludedRegion);
         requestPair = association(requester, requesterAttributes, requestRegion);
         requester.subscribeObjectClassAttributesWithRegions(
            requesterBase, requestPair, "");
         subscribed = true;

         ownerObjects.add(owner.registerObjectInstanceWithRegions(ownerBase, sourcePair));
         ownerObjects.add(owner.registerObjectInstanceWithRegions(ownerBase, sourcePair));
         ownerObjects.add(owner.registerObjectInstanceWithRegions(ownerDerived, derivedPair));
         ObjectInstanceHandle excludedObject =
            owner.registerObjectInstanceWithRegions(ownerBase, excludedPair);
         ownerObjects.add(excludedObject);
         ObjectInstanceHandle registeredRequesterObject =
            requester.registerObjectInstanceWithRegions(requesterBase, requestPair);
         requesterObject = registeredRequesterObject;
         JavaTckSupport.check(ownerObjects.stream().allMatch(RegionalClassRequestUpdateAcrossHierarchyTck::valid)
               && valid(registeredRequesterObject)
               && requester.isAttributeOwnedByFederate(
                  registeredRequesterObject, requesterBaseAttribute),
            "regional class-request registration returned an invalid instance or lost requester ownership");
         JavaTckSupport.check(ownerObjects.stream().distinct().count() == ownerObjects.size(),
            "regional class-request registration reused an object instance handle");

         ownerCallbacks.provided.clear();
         requesterCallbacks.provided.clear();
         byte[] requestTag = new byte[] {0x71, 0x52};
         requester.requestAttributeValueUpdateWithRegions(requesterBase, requestPair, requestTag);
         await(owner, requester, model, () -> ownerCallbacks.provided.size() >= 3,
            "regional base-class request to base and derived instances");
         List<ProvidedUpdate> requests = List.copyOf(ownerCallbacks.provided);
         JavaTckSupport.check(requests.size() == 3,
            "regional base-class request did not solicit exactly three overlapping instances");
         verifySolicitation(requests, ownerObjects.get(0), ownerBaseAttribute, requestTag);
         verifySolicitation(requests, ownerObjects.get(1), ownerBaseAttribute, requestTag);
         verifySolicitation(requests, ownerObjects.get(2), ownerDerivedAttribute, requestTag);
         JavaTckSupport.check(requests.stream().noneMatch(
               update -> excludedObject.equals(update.object)),
            "regional class request solicited an object outside the supplied region");
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(requester);
         JavaTckSupport.check(requesterCallbacks.provided.stream().noneMatch(
               update -> registeredRequesterObject.equals(update.object)),
            "regional request solicited an attribute owned by the requesting federate");

         requesterCallbacks.reflections.clear();
         List<ExpectedReflection> expected = new ArrayList<>();
         int responseIndex = 0;
         for (ProvidedUpdate request : requests) {
            byte[] responseValue = new byte[] {(byte) 0xa1, (byte) responseIndex};
            byte[] responseTag = new byte[] {0x73, (byte) responseIndex};
            AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
            for (AttributeHandle attribute : request.attributes) values.put(attribute, responseValue);
            owner.updateAttributeValues(request.object, values, responseTag);
            expected.add(new ExpectedReflection(request.object,
               request.attributes.iterator().next(), responseValue, responseTag));
            ++responseIndex;
         }
         await(requester, owner, model,
            () -> requesterCallbacks.reflections.size() >= expected.size(),
            "receive-order regional request responses");
         List<Reflection> reflections = List.copyOf(requesterCallbacks.reflections);
         JavaTckSupport.check(reflections.size() == expected.size()
               && reflections.stream().noneMatch(reflection -> excludedObject.equals(reflection.object)),
            "regional request returned an unexpected or excluded reflection");
         for (ExpectedReflection response : expected) {
            List<Reflection> matching = reflections.stream()
               .filter(reflection -> response.object.equals(reflection.object)).toList();
            JavaTckSupport.check(matching.size() == 1,
               "regional response did not reflect each solicited object exactly once");
            Reflection reflection = matching.get(0);
            JavaTckSupport.check(reflection.values.size() == 1
                  && Arrays.equals(response.value, reflection.values.get(response.attribute))
                  && Arrays.equals(response.tag, reflection.tag),
               "regional response changed the requested attribute value or response tag");
         }
      } finally {
         if (requesterJoined && subscribed && requestPair != null && requesterBase != null) try {
            requester.unsubscribeObjectClassAttributesWithRegions(requesterBase, requestPair);
         } catch (Exception ignored) { }
         if (requesterJoined && requesterObject != null && requestPair != null) try {
            requester.unassociateRegionsForUpdates(requesterObject, requestPair);
         } catch (Exception ignored) { }
         if (ownerJoined && sourcePair != null) for (int i = 0; i < Math.min(2, ownerObjects.size()); ++i) {
            ObjectInstanceHandle object = ownerObjects.get(i);
            try { owner.unassociateRegionsForUpdates(object, sourcePair); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerObjects.size() > 2 && derivedPair != null) try {
            owner.unassociateRegionsForUpdates(ownerObjects.get(2), derivedPair);
         } catch (Exception ignored) { }
         if (ownerJoined && ownerObjects.size() > 3 && excludedPair != null) try {
            owner.unassociateRegionsForUpdates(ownerObjects.get(3), excludedPair);
         } catch (Exception ignored) { }
         if (requesterJoined && requesterPublished && requesterBase != null
               && requesterAttributes != null) try {
            requester.unpublishObjectClassAttributes(requesterBase, requesterAttributes);
         } catch (Exception ignored) { }
         if (ownerJoined && ownerDerivedPublished && ownerDerived != null
               && ownerDerivedAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerDerived, ownerDerivedAttributes);
         } catch (Exception ignored) { }
         if (ownerJoined && ownerBasePublished && ownerBase != null
               && ownerBaseAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerBase, ownerBaseAttributes);
         } catch (Exception ignored) { }
         deleteRegion(owner, sourceRegion, sourceRegionCreated, ownerJoined);
         deleteRegion(owner, excludedRegion, excludedRegionCreated, ownerJoined);
         deleteRegion(requester, requestRegion, requestRegionCreated, requesterJoined);
         if (requesterJoined) try {
            requester.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         } catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); }
            catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static void timestampedResponse(RtiFactory factory, String fom,
         String timeImplementation, String firstDimensionName, String secondDimensionName,
         CallbackModel model) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-regional-request-tso-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false, published = false, subscribed = false;
      ObjectClassHandle ownerClass = null, requesterClass = null;
      AttributeHandle ownerAttribute = null, requesterAttribute = null;
      AttributeHandleSet ownerAttributes = null, requesterAttributes = null;
      AttributeSetRegionSetPairList ownerPair = null, requesterPair = null;
      RegionHandle ownerRegion = null, requesterRegion = null;
      boolean ownerRegionCreated = false, requesterRegionCreated = false;
      ObjectInstanceHandle object = null;
      FederateHandle producer = null;
      boolean constrained = false, regulating = false;
      try {
         owner.connect(ownerCallbacks.proxy(), model);
         ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), model);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         producer = owner.joinFederationExecution("regional-request-tso-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("regional-request-tso-requester", federation);
         requesterJoined = true;

         LogicalTimeFactory<?, ?> ownerTimeFactory = owner.getTimeFactory();
         LogicalTimeFactory<?, ?> requesterTimeFactory = requester.getTimeFactory();
         JavaTckSupport.check(ownerTimeFactory.getName().equals(requesterTimeFactory.getName()),
            "regional timestamped request members selected different logical-time factories");
         LogicalTime<?, ?> ownerInitial = (LogicalTime<?, ?>) ownerTimeFactory.makeInitial();
         LogicalTimeInterval<?> ownerEpsilon =
            (LogicalTimeInterval<?>) ownerTimeFactory.makeEpsilon();
         Object timeValue = ownerInitial;
         for (int i = 0; i < 4; ++i)
            timeValue = ((LogicalTime) timeValue).add((LogicalTimeInterval) ownerEpsilon);
         LogicalTime<?, ?> responseTime = (LogicalTime<?, ?>) timeValue;

         requester.enableTimeConstrained();
         await(owner, requester, model, () -> requesterCallbacks.constrainedEnabled,
            "regional requester time-constrained callback");
         constrained = true;
         owner.enableTimeRegulation((LogicalTimeInterval) ownerEpsilon);
         await(owner, requester, model, () -> ownerCallbacks.regulationEnabled,
            "regional owner time-regulation callback");
         regulating = true;

         ownerClass = owner.getObjectClassHandle(BASE_CLASS);
         requesterClass = requester.getObjectClassHandle(BASE_CLASS);
         ownerAttribute = owner.getAttributeHandle(ownerClass, "Name");
         requesterAttribute = requester.getAttributeHandle(requesterClass, "Name");
         Dimensions ownerDimensions = dimensions(owner, ownerClass, firstDimensionName,
            secondDimensionName, "timestamped owner class");
         Dimensions requesterDimensions = dimensions(requester, requesterClass,
            firstDimensionName, secondDimensionName, "timestamped requester class");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.changeDefaultAttributeOrderType(ownerClass, ownerAttributes, OrderType.TIMESTAMP);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         ownerRegion = createRegion(owner, ownerDimensions, 1, 5);
         ownerRegionCreated = true;
         requesterRegion = createRegion(requester, requesterDimensions, 2, 6);
         requesterRegionCreated = true;
         ownerPair = association(owner, ownerAttributes, ownerRegion);
         requesterPair = association(requester, requesterAttributes, requesterRegion);
         requester.subscribeObjectClassAttributesWithRegions(requesterClass, requesterPair, "");
         subscribed = true;
         ObjectInstanceHandle registeredObject =
            owner.registerObjectInstanceWithRegions(ownerClass, ownerPair);
         object = registeredObject;
         JavaTckSupport.check(valid(registeredObject),
            "regional timestamped request registration returned an invalid object handle");
         await(owner, requester, model,
            () -> requesterCallbacks.discoveries.contains(registeredObject),
            "regional timestamped request object discovery");

         byte[] requestTag = new byte[] {0x39, 0x71};
         ownerCallbacks.provided.clear();
         requesterCallbacks.timedReflections.clear();
         requester.requestAttributeValueUpdateWithRegions(
            requesterClass, requesterPair, requestTag);
         await(owner, requester, model, () -> ownerCallbacks.provided.size() >= 1,
            "regional timestamped provider callback");
         JavaTckSupport.check(ownerCallbacks.provided.size() == 1
               && object.equals(ownerCallbacks.provided.get(0).object)
               && ownerAttributes.equals(ownerCallbacks.provided.get(0).attributes)
               && Arrays.equals(requestTag, ownerCallbacks.provided.get(0).tag),
            "regional timestamped provider callback returned different object, attribute, or tag");

         byte[] responseValue = new byte[] {0x46, (byte) 0x92};
         byte[] responseTag = new byte[] {0x48, 0x21};
         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(ownerAttribute, responseValue);
         MessageRetractionReturn retraction = owner.updateAttributeValues(
            object, values, responseTag, responseTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "regional timestamped response returned no valid retraction handle");
         JavaTckSupport.check(requesterCallbacks.timedReflections.isEmpty(),
            "regional timestamped response was delivered before the time advance");
         requester.timeAdvanceRequest(responseTime);
         owner.timeAdvanceRequest(responseTime);
         await(owner, requester, model,
            () -> ownerCallbacks.grants.size() >= 1 && requesterCallbacks.grants.size() >= 1
               && requesterCallbacks.timedReflections.size() >= 1,
            "regional timestamped response reflection and time-advance grants");
         TimedReflection reflection = requesterCallbacks.timedReflections.get(0);
         JavaTckSupport.check(requesterCallbacks.timedReflections.size() == 1
               && object.equals(reflection.object)
               && reflection.values.size() == 1
               && Arrays.equals(responseValue, reflection.values.get(requesterAttribute))
               && Arrays.equals(responseTag, reflection.tag)
               && producer.equals(reflection.producer)
               && responseTime.equals(reflection.time)
               && OrderType.TIMESTAMP.equals(reflection.sentOrder)
               && OrderType.TIMESTAMP.equals(reflection.receivedOrder)
               && responseTime.equals(ownerCallbacks.grants.get(0))
               && responseTime.equals(requesterCallbacks.grants.get(0)),
            "regional request reflection lost the owner's timestamp, value, producer, or order");
      } finally {
         if (requesterJoined && subscribed && requesterClass != null && requesterPair != null) try {
            requester.unsubscribeObjectClassAttributesWithRegions(requesterClass, requesterPair);
         } catch (Exception ignored) { }
         if (ownerJoined && object != null && ownerPair != null) try {
            owner.unassociateRegionsForUpdates(object, ownerPair);
         } catch (Exception ignored) { }
         if (ownerJoined && published && ownerClass != null && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         deleteRegion(owner, ownerRegion, ownerRegionCreated, ownerJoined);
         deleteRegion(requester, requesterRegion, requesterRegionCreated, requesterJoined);
         if (requesterJoined) try { requester.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (ownerJoined) try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); }
            catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static Dimensions dimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         String firstName, String secondName, String description) throws Exception {
      DimensionHandle first = ambassador.getDimensionHandle(firstName);
      DimensionHandle second = ambassador.getDimensionHandle(secondName);
      Set<DimensionHandle> available = ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(!first.equals(second) && available.contains(first)
            && available.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         description + " did not expose the two adapter dimensions with sufficient range");
      return new Dimensions(first, second);
   }

   private static RegionHandle createRegion(RTIambassador ambassador, Dimensions dimensions,
         long lower, long upper) throws Exception {
      DimensionHandleSet regionDimensions = ambassador.getDimensionHandleSetFactory().create();
      regionDimensions.add(dimensions.first);
      regionDimensions.add(dimensions.second);
      RegionHandle region = ambassador.createRegion(regionDimensions);
      ambassador.setRangeBounds(region, dimensions.first, new RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, dimensions.second, new RangeBounds(lower, upper));
      RegionHandleSet commit = ambassador.getRegionHandleSetFactory().create();
      commit.add(region);
      ambassador.commitRegionModifications(commit);
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

   private static void verifySolicitation(List<ProvidedUpdate> requests,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] tag) {
      List<ProvidedUpdate> matching = requests.stream()
         .filter(request -> object.equals(request.object)).toList();
      JavaTckSupport.check(matching.size() == 1
            && matching.get(0).attributes.size() == 1
            && matching.get(0).attributes.contains(attribute)
            && Arrays.equals(tag, matching.get(0).tag),
         "regional class request changed the solicited object's attribute or request tag");
   }

   private static boolean valid(ObjectInstanceHandle handle) {
      return handle != null && handle.encodedLength() > 0;
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean created, boolean joined) {
      if (created && joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            JavaTckSupport.drain(second);
         } else Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Dimensions {
      private final DimensionHandle first;
      private final DimensionHandle second;
      private Dimensions(DimensionHandle first, DimensionHandle second) {
         this.first = first;
         this.second = second;
      }
   }

   private static final class ProvidedUpdate {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;
      private ProvidedUpdate(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag;
      }
   }

   private static class Reflection {
      final ObjectInstanceHandle object;
      final AttributeHandleValueMap values;
      final byte[] tag;
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag) {
         this.object = object;
         this.values = values;
         this.tag = tag;
      }
   }

   private static final class TimedReflection extends Reflection {
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder, receivedOrder;
      private TimedReflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, FederateHandle producer, LogicalTime<?, ?> time,
            OrderType sentOrder, OrderType receivedOrder) {
         super(object, values, tag);
         this.producer = producer;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
      }
   }

   private static final class ExpectedReflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandle attribute;
      private final byte[] value, tag;
      private ExpectedReflection(ObjectInstanceHandle object, AttributeHandle attribute,
            byte[] value, byte[] tag) {
         this.object = object;
         this.attribute = attribute;
         this.value = value;
         this.tag = tag;
      }
   }

   private static class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<TimedReflection> timedReflections = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private volatile boolean constrainedEnabled, regulationEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("provideAttributeValueUpdate".equals(name) && arguments != null
                     && arguments.length == 3) {
                  provided.add(new ProvidedUpdate((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) ((AttributeHandleSet) arguments[1]).clone(),
                     ((byte[]) arguments[2]).clone()));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     ((AttributeHandleValueMap) arguments[1]).clone(),
                     ((byte[]) arguments[2]).clone()));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timedReflections.add(new TimedReflection((ObjectInstanceHandle) arguments[0],
                     ((AttributeHandleValueMap) arguments[1]).clone(),
                     ((byte[]) arguments[2]).clone(), (FederateHandle) arguments[4],
                     (LogicalTime<?, ?>) arguments[6], (OrderType) arguments[7],
                     (OrderType) arguments[8]));
               } else if ("timeConstrainedEnabled".equals(name)) {
                  constrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  regulationEnabled = true;
               } else if ("timeAdvanceGrant".equals(name) && arguments != null) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional class-request callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
