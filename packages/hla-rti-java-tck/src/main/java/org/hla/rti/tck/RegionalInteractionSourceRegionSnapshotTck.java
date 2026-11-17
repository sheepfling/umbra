package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Preserves the send-time source region on queued ordinary regional interactions. */
final class RegionalInteractionSourceRegionSnapshotTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] VALUE = new byte[] {0x2C, 0x01};
   private static final byte[] TAG = new byte[] {0x53, 0x4E, 0x50};

   private RegionalInteractionSourceRegionSnapshotTck() { }

   static void run(RtiFactory factory, String fom, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-regional-interaction-source-snapshot-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false, published = false, subscribed = false;
      InteractionClassHandle publisherInteraction = null, subscriberInteraction = null;
      RegionHandle sourceRegion = null, subscriberRegion = null;
      RegionHandleSet sourceRegions = null, subscriberRegions = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producingFederate = publisher.joinFederationExecution(
            "java-tck-regional-snapshot-publisher", federation);
         publisherJoined = true;
         JavaTckSupport.check(producingFederate != null && producingFederate.encodedLength() > 0,
            "source-snapshot publisher join returned an invalid federate handle");
         subscriber.joinFederationExecution("java-tck-regional-snapshot-subscriber", federation);
         subscriberJoined = true;
         subscriberCallbacks.inspectRegionDesignatorsWith(subscriber);

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         subscriberInteraction = subscriber.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle subscriberParameter =
            subscriber.getParameterHandle(subscriberInteraction, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle subscriberFirst = subscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle subscriberSecond = subscriber.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherInteraction != null && publisherInteraction.encodedLength() > 0
               && subscriberInteraction != null && subscriberInteraction.encodedLength() > 0
               && publisherParameter != null && publisherParameter.encodedLength() > 0
               && subscriberParameter != null && subscriberParameter.encodedLength() > 0
               && publisherFirst != null && publisherFirst.encodedLength() > 0
               && publisherSecond != null && publisherSecond.encodedLength() > 0
               && subscriberFirst != null && subscriberFirst.encodedLength() > 0
               && subscriberSecond != null && subscriberSecond.encodedLength() > 0
               && !publisherFirst.equals(publisherSecond)
               && publisherInteraction.equals(subscriberInteraction)
               && publisherParameter.equals(subscriberParameter)
               && publisherFirst.equals(subscriberFirst) && publisherSecond.equals(subscriberSecond),
            "adapter DDM FOM returned inconsistent interaction, parameter, or dimension handles");
         checkDimensions(publisher, publisherInteraction, publisherFirst, publisherSecond,
            firstDimensionName, secondDimensionName);
         checkDimensions(subscriber, subscriberInteraction, subscriberFirst, subscriberSecond,
            firstDimensionName, secondDimensionName);

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5);
         subscriberRegion = createRegion(subscriber, subscriberFirst, subscriberSecond, 2, 6);
         sourceRegions = regionSet(publisher, sourceRegion);
         subscriberRegions = regionSet(subscriber, subscriberRegion);
         List<RegionDesignatorSnapshot> sourceDesignators =
            snapshotRegionDesignators(publisher, sourceRegions);
         subscriber.setConveyRegionDesignatorSetsSwitch(true);
         subscriber.subscribeInteractionClassWithRegions(
            subscriberInteraction, subscriberRegions);
         subscribed = true;

         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, VALUE.clone());
         publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, sourceRegions, TAG.clone());
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(subscriberCallbacks.interactions.isEmpty(),
               "evoked regional interaction arrived before callback servicing");
            moveSourceRegion(publisher, sourceRegion, publisherFirst, 0, 1);
         }
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.interactions.size() >= 1,
            "send-time source-region snapshot delivery");
         JavaTckSupport.check(subscriberCallbacks.interactions.size() == 1,
            "send-time source-region snapshot delivered a duplicate interaction");
         assertInteraction(subscriber, subscriberCallbacks.interactions.get(0),
            subscriberInteraction, subscriberParameter, producingFederate, sourceDesignators);

         subscriberCallbacks.interactions.clear();
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            moveSourceRegion(publisher, sourceRegion, publisherFirst, 0, 1);
         }
         publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, sourceRegions, TAG.clone());
         pumpSeveral(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.interactions.isEmpty(),
            "disjoint source region did not suppress ordinary interaction delivery");

         moveSourceRegion(publisher, sourceRegion, publisherFirst, 1, 5);
         publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, sourceRegions, TAG.clone());
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.interactions.size() >= 1,
            "delivery after source-region restoration");
         JavaTckSupport.check(subscriberCallbacks.interactions.size() == 1,
            "restored source region delivered a duplicate interaction");
         assertInteraction(subscriber, subscriberCallbacks.interactions.get(0),
            subscriberInteraction, subscriberParameter, producingFederate, sourceDesignators);

         subscriber.unsubscribeInteractionClassWithRegions(
            subscriberInteraction, subscriberRegions);
         subscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         subscriber.deleteRegion(subscriberRegion);
         subscriberRegion = null;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && subscriberJoined && subscriberRegions != null) try {
            subscriber.unsubscribeInteractionClassWithRegions(
               subscriberInteraction, subscriberRegions);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         deleteRegion(publisher, sourceRegion, publisherJoined);
         deleteRegion(subscriber, subscriberRegion, subscriberJoined);
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (subscriberJoined) try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador,
         InteractionClassHandle interaction, DimensionHandle first, DimensionHandle second,
         String firstName, String secondName)
         throws Exception {
      Set<DimensionHandle> available =
         ambassador.getAvailableDimensionsForInteractionClass(interaction);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && firstName.equals(ambassador.getDimensionName(first))
            && secondName.equals(ambassador.getDimensionName(second))
            && ambassador.getDimensionUpperBound(first) >= 6
            && ambassador.getDimensionUpperBound(second) >= 6,
         "adapter interaction FOM does not provide the named dimensions/bounds required for source snapshot");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      JavaTckSupport.check(region != null && region.encodedLength() > 0,
         "source-snapshot fixture returned an invalid region handle");
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = regionSet(ambassador, region);
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static List<RegionDesignatorSnapshot> snapshotRegionDesignators(
         RTIambassador ambassador, RegionHandleSet regions) throws Exception {
      List<RegionDesignatorSnapshot> result = new ArrayList<>();
      for (RegionHandle region : regions) {
         JavaTckSupport.check(region != null && region.encodedLength() > 0,
            "interaction callback returned an invalid source-region handle");
         Set<String> dimensions = new HashSet<>();
         Map<String, RangeSnapshot> ranges = new HashMap<>();
         for (DimensionHandle dimension : ambassador.getDimensionHandleSet(region)) {
            String name = ambassador.getDimensionName(dimension);
            RangeBounds bounds = ambassador.getRangeBounds(region, dimension);
            dimensions.add(name);
            ranges.put(name, new RangeSnapshot(bounds.lower, bounds.upper));
         }
         result.add(new RegionDesignatorSnapshot(dimensions, ranges));
      }
      return result;
   }

   private static void moveSourceRegion(RTIambassador publisher, RegionHandle sourceRegion,
         DimensionHandle dimension, long lower, long upper) throws Exception {
      publisher.setRangeBounds(
         sourceRegion, dimension, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = regionSet(publisher, sourceRegion);
      publisher.commitRegionModifications(regions);
   }

   private static void assertInteraction(RTIambassador receiver, Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, List<RegionDesignatorSnapshot> expectedSourceRegions)
         throws Exception {
      JavaTckSupport.check(expectedClass.equals(interaction.interaction)
            && interaction.parameters.size() == 1
            && Arrays.equals(VALUE, interaction.parameters.get(expectedParameter))
            && Arrays.equals(TAG, interaction.tag)
            && expectedProducer.equals(interaction.producer),
         "regional interaction snapshot lost class, value, tag, or producer metadata");
      TransportationTypeHandle transportation = interaction.transportation;
      JavaTckSupport.check(transportation != null
            && !receiver.getTransportationTypeName(transportation).isEmpty(),
         "regional interaction snapshot returned an unresolvable transportation type");
      List<RegionDesignatorSnapshot> actualSourceRegions = interaction.regionDesignators;
      JavaTckSupport.check(interaction.regions != null && interaction.regions.size() == 1
            && actualSourceRegions != null && actualSourceRegions.size() == 1
            && expectedSourceRegions.equals(actualSourceRegions),
         "regional interaction did not preserve its send-time source-region dimensions and bounds");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(ambassador, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpSeveral(RTIambassador ambassador, CallbackModel model) throws Exception {
      for (int pass = 0; pass < 8; ++pass) pump(ambassador, model);
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

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK regional snapshot publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private Set<RegionHandle> regions;
      private List<RegionDesignatorSnapshot> regionDesignators;
   }

   private static final class RegionDesignatorSnapshot {
      private final Set<String> dimensions;
      private final Map<String, RangeSnapshot> ranges;

      private RegionDesignatorSnapshot(Set<String> dimensions, Map<String, RangeSnapshot> ranges) {
         this.dimensions = dimensions;
         this.ranges = ranges;
      }

      @Override public boolean equals(Object other) {
         if (!(other instanceof RegionDesignatorSnapshot)) return false;
         RegionDesignatorSnapshot that = (RegionDesignatorSnapshot) other;
         return dimensions.equals(that.dimensions) && ranges.equals(that.ranges);
      }

      @Override public int hashCode() { return Objects.hash(dimensions, ranges); }
   }

   private static final class RangeSnapshot {
      private final long lower;
      private final long upper;

      private RangeSnapshot(long lower, long upper) {
         this.lower = lower;
         this.upper = upper;
      }

      @Override public boolean equals(Object other) {
         if (!(other instanceof RangeSnapshot)) return false;
         RangeSnapshot that = (RangeSnapshot) other;
         return lower == that.lower && upper == that.upper;
      }

      @Override public int hashCode() { return Objects.hash(lower, upper); }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private volatile RTIambassador regionInspector;

      private void inspectRegionDesignatorsWith(RTIambassador ambassador) {
         regionInspector = ambassador;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  Interaction interaction = new Interaction();
                  interaction.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
                  interaction.parameters = new HashMap<>();
                  values.forEach((handle, value) -> interaction.parameters.put(handle, value.clone()));
                  interaction.tag = ((byte[]) arguments[2]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[3];
                  interaction.producer = (FederateHandle) arguments[4];
                  interaction.regions = arguments[5] == null ? null
                     : new HashSet<>((RegionHandleSet) arguments[5]);
                  if (arguments[5] != null && regionInspector != null) {
                     interaction.regionDesignators =
                        snapshotRegionDesignators(regionInspector, (RegionHandleSet) arguments[5]);
                  }
                  interactions.add(interaction);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional source snapshot callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
