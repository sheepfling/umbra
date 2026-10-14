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

/** Timestamped regional attribute delivery through FQR, TARA, and NMRA. */
final class TimestampedRegionalAttributeAlternateAdvancesTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedRegionalAttributeAlternateAdvancesTck() { }

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
      RTIambassador[] ambassadors = new RTIambassador[4];
      Recorder[] recorders = new Recorder[4];
      String federation = "java-tck-tso-attribute-alternates-" + UUID.randomUUID();
      boolean[] connected = new boolean[4];
      boolean[] joined = new boolean[4];
      boolean[] subscribed = new boolean[4];
      RegionHandle[] regions = new RegionHandle[4];
      AttributeSetRegionSetPairList[] pairs = new AttributeSetRegionSetPairList[4];
      ObjectClassHandle[] classes = new ObjectClassHandle[4];
      AttributeHandle[] attributes = new AttributeHandle[4];
      AttributeHandleSet[] attributeSets = new AttributeHandleSet[4];
      boolean created = false, published = false, regulating = false;
      boolean[] constrained = new boolean[4];
      try {
         for (int i = 0; i < 4; ++i) {
            ambassadors[i] = factory.getRtiAmbassador();
            recorders[i] = new Recorder();
            ambassadors[i].connect(recorders[i].proxy(), callbackModel);
            connected[i] = true;
         }
         ambassadors[0].createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = ambassadors[0].joinFederationExecution(
            "java-tck-regional-alternate-producer", federation);
         joined[0] = true;
         for (int i = 1; i < 4; ++i) {
            ambassadors[i].joinFederationExecution(
               "java-tck-regional-alternate-receiver-" + i, federation);
            joined[i] = true;
         }

         DimensionHandle[] first = new DimensionHandle[4];
         DimensionHandle[] second = new DimensionHandle[4];
         for (int i = 0; i < 4; ++i) {
            classes[i] = ambassadors[i].getObjectClassHandle(objectClassName);
            attributes[i] = ambassadors[i].getAttributeHandle(classes[i], attributeName);
            first[i] = ambassadors[i].getDimensionHandle(firstDimensionName);
            second[i] = ambassadors[i].getDimensionHandle(secondDimensionName);
            attributeSets[i] = attributeSet(ambassadors[i], attributes[i]);
            checkDimensions(ambassadors[i], classes[i], first[i], second[i], defaultRegion ? 6 : 9);
         }
         ambassadors[0].publishObjectClassAttributes(classes[0], attributeSets[0]);
         published = true;
         ambassadors[0].changeDefaultAttributeOrderType(
            classes[0], attributeSets[0], OrderType.TIMESTAMP);
         for (int i = 0; i < 4; ++i) {
            long low = i == 0 ? 1 : (i == 1 ? 2 : (i == 2 ? 2 : 2));
            long high = i == 0 ? 5 : 6;
            if (i != 0 || !defaultRegion) {
               regions[i] = createRegion(ambassadors[i], first[i], second[i], low, high, low, high);
               pairs[i] = association(ambassadors[i], attributeSets[i], regions[i]);
            }
            if (i > 0) {
               ambassadors[i].setConveyRegionDesignatorSetsSwitch(true);
               JavaTckSupport.check(ambassadors[i].getConveyRegionDesignatorSetsSwitch(),
                  "regional alternate receiver did not enable region designators");
               ambassadors[i].subscribeObjectClassAttributesWithRegions(classes[i], pairs[i]);
               subscribed[i] = true;
            }
         }
         ObjectInstanceHandle object = defaultRegion
            ? ambassadors[0].registerObjectInstance(classes[0])
            : ambassadors[0].registerObjectInstanceWithRegions(classes[0], pairs[0]);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "timestamped regional alternate registration returned an invalid handle");
         await(ambassadors, callbackModel, () -> recorders[1].discoveries.contains(object)
            && recorders[2].discoveries.contains(object) && recorders[3].discoveries.contains(object),
            "three regional alternate object discoveries");

         LogicalTimeFactory<?, ?> timeFactory = ambassadors[0].getTimeFactory();
         LogicalTimeFactory<?, ?>[] factories = new LogicalTimeFactory<?, ?>[4];
         for (int i = 0; i < 4; ++i) {
            factories[i] = ambassadors[i].getTimeFactory();
            JavaTckSupport.check(timeFactory.getName().equals(factories[i].getName()),
               "regional alternate members selected different logical-time factories");
         }
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         for (int i = 1; i < 4; ++i) {
            ambassadors[i].enableTimeConstrained();
            constrained[i] = true;
         }
         ambassadors[0].enableTimeRegulation(epsilon);
         regulating = true;
         await(ambassadors, callbackModel, () -> recorders[0].timeRegulationEnabled
            && recorders[1].timeConstrainedEnabled && recorders[2].timeConstrainedEnabled
            && recorders[3].timeConstrainedEnabled, "regional alternate time roles");

         LogicalTime initial = ambassadors[0].queryLogicalTime();
         for (int i = 1; i < 4; ++i) {
            JavaTckSupport.check(initial.equals(ambassadors[i].queryLogicalTime()),
               "regional alternate members began at different logical times");
         }
         LogicalTime messageTime = offset(initial, epsilon, 3);
         LogicalTime publisherTarget = offset(initial, epsilon, 2);
         LogicalTime requestBoundary = offset(initial, epsilon, 5);
         byte[] value = defaultRegion
            ? new byte[] {0x44, 0x45, 0x46, 0x2d, 0x41}
            : new byte[] {0x61, 0x62, 0x63};
         byte[] tag = defaultRegion
            ? new byte[] {0x44, 0x45, 0x46, 0x2d, 0x46}
            : new byte[] {0x52, 0x41, 0x41};
         AttributeHandleValueMap values =
            ambassadors[0].getAttributeHandleValueMapFactory().create(1);
         values.put(attributes[0], value.clone());
         MessageRetractionReturn retraction =
            ambassadors[0].updateAttributeValues(object, values, tag.clone(), messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped regional alternate update returned an invalid retraction handle");
         for (int i = 1; i < 4; ++i) {
            pump(ambassadors[i], callbackModel);
            JavaTckSupport.check(recorders[i].reflections.isEmpty(),
               "regional alternate receiver got a reflection before requesting an advance");
            recorders[i].callbackOrder.clear();
         }
         recorders[0].callbackOrder.clear();

         ambassadors[1].flushQueueRequest(requestBoundary);
         ambassadors[2].timeAdvanceRequestAvailable(messageTime);
         ambassadors[3].nextMessageRequestAvailable(requestBoundary);
         ambassadors[0].timeAdvanceRequest(publisherTarget);
         await(ambassadors, callbackModel, () -> recorders[0].grants.size() >= 1
            && recorders[1].reflections.size() >= 1 && recorders[1].flushGrants.size() >= 1
            && recorders[2].reflections.size() >= 1 && recorders[2].grants.size() >= 1
            && recorders[3].reflections.size() >= 1 && recorders[3].grants.size() >= 1,
            "regional alternate delivery and grants");

         for (int i = 1; i < 4; ++i) {
            JavaTckSupport.check(recorders[i].reflections.size() == 1,
               "regional alternate recipient got the wrong reflection count");
            assertReflection(ambassadors[i], recorders[i].reflections.get(0), object,
               attributes[i], producer, value, tag, messageTime, retraction.handle,
               defaultRegion, regions[0]);
         }
         JavaTckSupport.check(recorders[0].grants.size() == 1
               && publisherTarget.equals(recorders[0].grants.get(0)),
            "producer TAR returned the wrong grant time");
         JavaTckSupport.check(recorders[1].flushGrants.size() == 1
               && recorders[1].grants.isEmpty()
               && recorders[1].callbackOrder.equals(Arrays.asList("reflect", "flush-grant")),
            "Flush Queue Request returned the wrong callback pair or order");
         JavaTckSupport.check(recorders[2].flushGrants.isEmpty()
               && recorders[2].grants.size() == 1
               && recorders[2].callbackOrder.equals(Arrays.asList("reflect", "grant")),
            "TARA returned the wrong callback pair or order");
         JavaTckSupport.check(recorders[3].flushGrants.isEmpty()
               && recorders[3].grants.size() == 1
               && recorders[3].callbackOrder.equals(Arrays.asList("reflect", "grant")),
            "NMRA returned the wrong callback pair or order");
         JavaTckSupport.check(messageTime.equals(recorders[2].grants.get(0))
               && messageTime.equals(recorders[3].grants.get(0)),
            "TARA or NMRA returned the wrong grant time");
         LogicalTime fqGrant = recorders[1].flushGrants.get(0).grant;
         LogicalTime fqOptimistic = recorders[1].flushGrants.get(0).optimistic;
         JavaTckSupport.check(messageTime.equals(fqOptimistic)
               && compare(fqGrant, initial) >= 0 && compare(fqGrant, requestBoundary) <= 0
               && compare(fqOptimistic, fqGrant) >= 0
               && fqGrant.equals(ambassadors[1].queryLogicalTime()),
            "Flush Queue Grant returned inconsistent actual or optimistic time");
         for (int i = 2; i < 4; ++i) {
            JavaTckSupport.check(messageTime.equals(ambassadors[i].queryLogicalTime()),
               "alternate advance logical-time query disagreed with its grant");
         }
         JavaTckSupport.check(publisherTarget.equals(ambassadors[0].queryLogicalTime()),
            "producer logical-time query disagreed with its grant");

         for (int i = 1; i < 4; ++i) {
            LogicalTime past = offset(factories[i], epsilon, 1);
            if (i == 1) {
               expectFailure(() -> ambassadors[1].flushQueueRequest(past),
                  "LogicalTimeAlreadyPassed", "past-time FQR");
            } else if (i == 2) {
               expectFailure(() -> ambassadors[2].timeAdvanceRequestAvailable(past),
                  "LogicalTimeAlreadyPassed", "past-time TARA");
            } else {
               expectFailure(() -> ambassadors[3].nextMessageRequestAvailable(past),
                  "LogicalTimeAlreadyPassed", "past-time NMRA");
            }
         }
         expectFailure(() -> ambassadors[0].retract(retraction.handle),
            "MessageCanNoLongerBeRetracted", "retract after regional alternate delivery");

         for (int i = 1; i < 4; ++i) {
            ambassadors[i].disableTimeConstrained();
            constrained[i] = false;
            ambassadors[i].unsubscribeObjectClassAttributesWithRegions(classes[i], pairs[i]);
            subscribed[i] = false;
         }
         ambassadors[0].disableTimeRegulation();
         regulating = false;
         if (!defaultRegion) ambassadors[0].unassociateRegionsForUpdates(object, pairs[0]);
         ambassadors[0].unpublishObjectClassAttributes(classes[0], attributeSets[0]);
         published = false;
         for (int i = 0; i < 4; ++i) {
            if (regions[i] != null) ambassadors[i].deleteRegion(regions[i]);
            regions[i] = null;
         }
      } finally {
         for (int i = 1; i < 4; ++i) {
            if (constrained[i] && joined[i]) try { ambassadors[i].disableTimeConstrained(); }
               catch (Exception ignored) { }
            if (subscribed[i] && joined[i]) try {
               ambassadors[i].unsubscribeObjectClassAttributesWithRegions(classes[i], pairs[i]);
            } catch (Exception ignored) { }
         }
         if (regulating && joined[0]) try { ambassadors[0].disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (published && joined[0]) try {
            ambassadors[0].unpublishObjectClassAttributes(classes[0], attributeSets[0]);
         } catch (Exception ignored) { }
         for (int i = 3; i >= 0; --i) {
            if (regions[i] != null && joined[i]) try { ambassadors[i].deleteRegion(regions[i]); }
               catch (Exception ignored) { }
         }
         for (int i = 3; i >= 0; --i) {
            if (joined[i]) try { ambassadors[i].resignFederationExecution(ResignAction.NO_ACTION); }
               catch (Exception ignored) { }
         }
         if (created) try { ambassadors[0].destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         for (int i = 3; i >= 0; --i) {
            if (connected[i]) try { ambassadors[i].disconnect(); }
               catch (Exception ignored) { }
         }
      }
   }

   private static AttributeHandleSet attributeSet(RTIambassador ambassador, AttributeHandle handle)
         throws Exception {
      AttributeHandleSet result = ambassador.getAttributeHandleSetFactory().create();
      result.add(handle);
      return result;
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second, long minimumUpperBound) throws Exception {
      java.util.Set<DimensionHandle> dimensions =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(dimensions.contains(first) && dimensions.contains(second)
            && ambassador.getDimensionUpperBound(first) >= minimumUpperBound
            && ambassador.getDimensionUpperBound(second) >= minimumUpperBound,
         "adapter FOM object class does not expose two usable DDM dimensions");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long low, long high, long secondLow, long secondHigh)
         throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      RegionHandleSet set = ambassador.getRegionHandleSetFactory().create();
      set.add(region);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(low, high));
      ambassador.setRangeBounds(region, second,
         new hla.rti1516_2025.RangeBounds(secondLow, secondHigh));
      ambassador.commitRegionModifications(set);
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

   private static void assertReflection(RTIambassador ambassador, Reflection actual,
         ObjectInstanceHandle object, AttributeHandle attribute, FederateHandle producer,
         byte[] value, byte[] tag, LogicalTime<?, ?> time,
         MessageRetractionHandle retraction, boolean defaultRegion, RegionHandle sourceRegion)
         throws Exception {
      JavaTckSupport.check(object.equals(actual.object) && actual.values.size() == 1
            && Arrays.equals(value, actual.values.get(attribute)) && Arrays.equals(tag, actual.tag)
            && producer.equals(actual.producer) && time.equals(actual.time)
            && actual.sentOrder == OrderType.TIMESTAMP && actual.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(actual.retraction) && actual.regions != null
            && (defaultRegion ? actual.regions.isEmpty()
               : actual.regions.size() == 1 && actual.regions.contains(sourceRegion)),
         "regional alternate reflection returned incorrect value or metadata");
      TransportationTypeHandle transportation = actual.transportation;
      JavaTckSupport.check(transportation != null
            && !ambassador.getTransportationTypeName(transportation).isEmpty(),
         "regional alternate reflection returned an unresolvable transport");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTime offset(LogicalTimeFactory<?, ?> factory,
         LogicalTimeInterval interval, int steps) throws Exception {
      return offset((LogicalTime) factory.makeInitial(), interval, steps);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compare(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   private static void await(RTIambassador[] ambassadors, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         for (RTIambassador ambassador : ambassadors) pump(ambassador, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void expectFailure(JavaTckSupport.CheckedOperation operation,
         String expected, String description) throws Exception {
      JavaTckSupport.expectFailure(operation, expected, description);
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

   private static final class FlushGrant {
      private final LogicalTime<?, ?> grant, optimistic;
      private FlushGrant(LogicalTime<?, ?> grant, LogicalTime<?, ?> optimistic) {
         this.grant = grant;
         this.optimistic = optimistic;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<FlushGrant> flushGrants = new CopyOnWriteArrayList<>();
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
               } else if ("timeAdvanceGrant".equals(name) && arguments != null) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               } else if ("flushQueueGrant".equals(name) && arguments != null
                     && arguments.length == 2) {
                  FlushGrant grant = new FlushGrant((LogicalTime<?, ?>) arguments[0],
                     (LogicalTime<?, ?>) arguments[1]);
                  flushGrants.add(grant);
                  callbackOrder.add("flush-grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional alternate callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
