package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
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

/** Verifies union delivery and de-duplication across multi-region interaction routes. */
final class MultiRegionInteractionRoutingTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final int PUBLISHER = 0;
   private static final int FIRST = 1;
   private static final int SECOND = 2;
   private static final int COMBINED = 3;
   private static final int DISJOINT = 4;
   private static final int NO_COMMON = 5;

   private MultiRegionInteractionRoutingTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String interactionName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador[] members = new RTIambassador[6];
      Recorder[] recorders = new Recorder[6];
      for (int i = 0; i < members.length; ++i) {
         members[i] = factory.getRtiAmbassador();
         recorders[i] = new Recorder();
      }
      String federation = "java-tck-multi-region-interaction-routing-" + UUID.randomUUID();
      boolean[] connected = new boolean[members.length];
      boolean[] joined = new boolean[members.length];
      boolean[] subscribed = new boolean[members.length];
      boolean created = false, published = false;
      RegionHandle[][] regions = new RegionHandle[members.length][];
      RegionHandle noCommonSource = null;
      FederateHandle[] federates = new FederateHandle[members.length];
      InteractionClassHandle[] interactions = new InteractionClassHandle[members.length];
      ParameterHandle[] parameters = new ParameterHandle[members.length];
      try {
         members[PUBLISHER].connect(noopFederateAmbassador(), callbackModel);
         connected[PUBLISHER] = true;
         for (int i = FIRST; i < members.length; ++i) {
            members[i].connect(recorders[i].proxy(), callbackModel);
            connected[i] = true;
         }
         members[PUBLISHER].createFederationExecution(federation, fom);
         created = true;
         federates[PUBLISHER] = members[PUBLISHER].joinFederationExecution(
            "java-tck-multi-region-publisher", federation);
         joined[PUBLISHER] = true;
         for (int i = FIRST; i < members.length; ++i) {
            federates[i] = members[i].joinFederationExecution(
               "java-tck-multi-region-member-" + i, federation);
            joined[i] = true;
         }

         ObjectClassHandle[] objectClasses = new ObjectClassHandle[members.length];
         AttributeHandle[] attributes = new AttributeHandle[members.length];
         DimensionHandle[] firstDimensions = new DimensionHandle[members.length];
         DimensionHandle[] secondDimensions = new DimensionHandle[members.length];
         for (int i = 0; i < members.length; ++i) {
            objectClasses[i] = members[i].getObjectClassHandle(objectClassName);
            attributes[i] = members[i].getAttributeHandle(objectClasses[i], attributeName);
            interactions[i] = members[i].getInteractionClassHandle(interactionName);
            parameters[i] = members[i].getParameterHandle(interactions[i], parameterName);
            firstDimensions[i] = members[i].getDimensionHandle(firstDimensionName);
            secondDimensions[i] = members[i].getDimensionHandle(secondDimensionName);
            JavaTckSupport.check(federates[i] != null && federates[i].encodedLength() > 0
                  && objectClasses[i] != null && objectClasses[i].encodedLength() > 0
                  && attributes[i] != null && attributes[i].encodedLength() > 0
                  && interactions[i] != null && interactions[i].encodedLength() > 0
                  && parameters[i] != null && parameters[i].encodedLength() > 0
                  && firstDimensions[i] != null && firstDimensions[i].encodedLength() > 0
                  && secondDimensions[i] != null && secondDimensions[i].encodedLength() > 0,
               "multi-region member resolved an invalid federate, declaration, or dimension handle");
            Set<DimensionHandle> available =
               members[i].getAvailableDimensionsForInteractionClass(interactions[i]);
            JavaTckSupport.check(available.contains(firstDimensions[i])
                  && available.contains(secondDimensions[i])
                  && members[i].getDimensionUpperBound(firstDimensions[i]) >= 9
                  && members[i].getDimensionUpperBound(secondDimensions[i]) >= 9,
               "adapter interaction FOM must expose both dimensions with ranges through 9");
         }
         for (int i = FIRST; i < members.length; ++i) {
            JavaTckSupport.check(interactions[PUBLISHER].equals(interactions[i])
                  && parameters[PUBLISHER].equals(parameters[i])
                  && firstDimensions[PUBLISHER].equals(firstDimensions[i])
                  && secondDimensions[PUBLISHER].equals(secondDimensions[i]),
               "multi-region federates resolved different interaction or parameter handles");
         }
         JavaTckSupport.check(!firstDimensions[PUBLISHER].equals(secondDimensions[PUBLISHER]),
            "multi-region interaction FOM resolved the two dimensions to one handle");
         Set<DimensionHandle> objectDimensions =
            members[PUBLISHER].getAvailableDimensionsForObjectClass(objectClasses[PUBLISHER]);
         JavaTckSupport.check(objectDimensions.contains(firstDimensions[PUBLISHER])
               && objectDimensions.contains(secondDimensions[PUBLISHER]),
            "multi-region object class omitted an adapter-supplied dimension");
         JavaTckSupport.check(firstDimensionName.equals(
                  members[PUBLISHER].getDimensionName(firstDimensions[PUBLISHER]))
               && secondDimensionName.equals(
                  members[PUBLISHER].getDimensionName(secondDimensions[PUBLISHER])),
            "multi-region dimension lookup returned the wrong configured name");
         JavaTckSupport.check(!members[PUBLISHER].getAllowRelaxedDDMSwitch()
               && !members[FIRST].getAllowRelaxedDDMSwitch(),
            "strict multi-region boundary checks require Allow Relaxed DDM to be disabled");
         members[PUBLISHER].publishInteractionClass(interactions[PUBLISHER]);
         published = true;

         regions[PUBLISHER] = new RegionHandle[2];
         regions[PUBLISHER][0] = createRegion(members[PUBLISHER], firstDimensions[PUBLISHER],
            secondDimensions[PUBLISHER], 0, 2, 0, 2);
         regions[PUBLISHER][1] = createRegion(members[PUBLISHER], firstDimensions[PUBLISHER],
            secondDimensions[PUBLISHER], 5, 7, 5, 7);
         regions[FIRST] = new RegionHandle[] {
            createRegion(members[FIRST], firstDimensions[FIRST], secondDimensions[FIRST], 0, 2, 0, 2)
         };
         regions[SECOND] = new RegionHandle[] {
            createRegion(members[SECOND], firstDimensions[SECOND], secondDimensions[SECOND], 5, 7, 5, 7)
         };
         regions[COMBINED] = new RegionHandle[2];
         regions[COMBINED][0] = createRegion(members[COMBINED], firstDimensions[COMBINED],
            secondDimensions[COMBINED], 0, 2, 0, 2);
         regions[COMBINED][1] = createRegion(members[COMBINED], firstDimensions[COMBINED],
            secondDimensions[COMBINED], 5, 7, 5, 7);
         regions[DISJOINT] = new RegionHandle[] {
            createRegion(members[DISJOINT], firstDimensions[DISJOINT], secondDimensions[DISJOINT],
               8, 9, 8, 9)
         };

         for (int i = FIRST; i <= DISJOINT; ++i) {
            members[i].setConveyRegionDesignatorSetsSwitch(true);
            JavaTckSupport.check(members[i].getConveyRegionDesignatorSetsSwitch(),
               "multi-region receiver did not enable source-region designators");
            members[i].subscribeInteractionClassWithRegions(interactions[i],
               regionSet(members[i], regions[i]));
            subscribed[i] = true;
         }
         for (int i = FIRST; i < members.length; ++i) {
            boolean verifyRegionDimensions = i == FIRST || i == SECOND || i == COMBINED;
            recorders[i].inspectRegionDesignatorsWith(members[i],
               verifyRegionDimensions ? firstDimensions[i] : null,
               verifyRegionDimensions ? secondDimensions[i] : null);
         }

         RegionHandleSet bothSources = regionSet(members[PUBLISHER], regions[PUBLISHER]);
         RegionHandleSet firstSource = regionSet(members[PUBLISHER], regions[PUBLISHER][0]);
         RegionHandleSet secondSource = regionSet(members[PUBLISHER], regions[PUBLISHER][1]);
         List<RegionDesignatorSnapshot> bothSourceDesignators =
            snapshotRegionDesignators(members[PUBLISHER], bothSources,
               firstDimensions[PUBLISHER], secondDimensions[PUBLISHER]);
         List<RegionDesignatorSnapshot> firstSourceDesignators =
            snapshotRegionDesignators(members[PUBLISHER], firstSource,
               firstDimensions[PUBLISHER], secondDimensions[PUBLISHER]);
         List<RegionDesignatorSnapshot> secondSourceDesignators =
            snapshotRegionDesignators(members[PUBLISHER], secondSource,
               firstDimensions[PUBLISHER], secondDimensions[PUBLISHER]);
         byte[] bothValue = new byte[] {0x61, 0x62, 0x63};
         byte[] bothTag = new byte[] {0x49, 0x31};
         send(members[PUBLISHER], interactions[PUBLISHER], parameters[PUBLISHER],
            bothSources, bothValue, bothTag);
         await(members, callbackModel, recorders, new int[] {0, 1, 1, 1, 0, 0},
            "multi-region interaction union delivery");
         assertDelivery(members[FIRST], recorders[FIRST].interactions.get(0), interactions[FIRST],
            parameters[FIRST], federates[PUBLISHER], bothValue, bothTag, bothSourceDesignators, "first route");
         assertDelivery(members[SECOND], recorders[SECOND].interactions.get(0), interactions[SECOND],
            parameters[SECOND], federates[PUBLISHER], bothValue, bothTag, bothSourceDesignators, "second route");
         assertDelivery(members[COMBINED], recorders[COMBINED].interactions.get(0),
            interactions[COMBINED], parameters[COMBINED], federates[PUBLISHER], bothValue, bothTag,
            bothSourceDesignators, "combined route without duplicate delivery");

         byte[] secondValue = new byte[] {0x71, 0x72};
         byte[] secondTag = new byte[] {0x49, 0x32};
         send(members[PUBLISHER], interactions[PUBLISHER], parameters[PUBLISHER],
            secondSource, secondValue, secondTag);
         await(members, callbackModel, recorders, new int[] {0, 1, 2, 2, 0, 0},
            "second-source interaction delivery");
         assertDelivery(members[SECOND], recorders[SECOND].interactions.get(1), interactions[SECOND],
            parameters[SECOND], federates[PUBLISHER], secondValue, secondTag, secondSourceDesignators,
            "second-source route");
         assertDelivery(members[COMBINED], recorders[COMBINED].interactions.get(1),
            interactions[COMBINED], parameters[COMBINED], federates[PUBLISHER], secondValue, secondTag,
            secondSourceDesignators, "combined second-source route");

         byte[] firstValue = new byte[] {(byte) 0x81, (byte) 0x82};
         byte[] firstTag = new byte[] {0x49, 0x33};
         send(members[PUBLISHER], interactions[PUBLISHER], parameters[PUBLISHER],
            firstSource, firstValue, firstTag);
         await(members, callbackModel, recorders, new int[] {0, 2, 2, 3, 0, 0},
            "first-source interaction delivery");
         assertDelivery(members[FIRST], recorders[FIRST].interactions.get(1), interactions[FIRST],
            parameters[FIRST], federates[PUBLISHER], firstValue, firstTag, firstSourceDesignators,
            "first-source route");
         assertDelivery(members[COMBINED], recorders[COMBINED].interactions.get(2),
            interactions[COMBINED], parameters[COMBINED], federates[PUBLISHER], firstValue, firstTag,
            firstSourceDesignators, "combined first-source route");

         members[FIRST].setRangeBounds(regions[FIRST][0], firstDimensions[FIRST],
            new RangeBounds(2, 4));
         members[FIRST].commitRegionModifications(regionSet(members[FIRST], regions[FIRST]));
         byte[] touchingValue = new byte[] {(byte) 0x91};
         byte[] touchingTag = new byte[] {0x49, 0x34};
         send(members[PUBLISHER], interactions[PUBLISHER], parameters[PUBLISHER], firstSource,
            touchingValue, touchingTag);
         await(members, callbackModel, recorders, new int[] {0, 2, 2, 4, 0, 0},
            "strict multi-region touching-boundary suppression");
         assertDelivery(members[COMBINED], recorders[COMBINED].interactions.get(3),
            interactions[COMBINED], parameters[COMBINED], federates[PUBLISHER], touchingValue, touchingTag,
            firstSourceDesignators, "combined strict touching-boundary control");

         members[FIRST].setRangeBounds(regions[FIRST][0], firstDimensions[FIRST],
            new RangeBounds(1, 3));
         members[FIRST].commitRegionModifications(regionSet(members[FIRST], regions[FIRST]));
         byte[] overlappingValue = new byte[] {(byte) 0x92};
         byte[] overlappingTag = new byte[] {0x49, 0x35};
         send(members[PUBLISHER], interactions[PUBLISHER], parameters[PUBLISHER], firstSource,
            overlappingValue, overlappingTag);
         await(members, callbackModel, recorders, new int[] {0, 3, 2, 5, 0, 0},
            "strict multi-region interior-overlap delivery");
         assertDelivery(members[FIRST], recorders[FIRST].interactions.get(2), interactions[FIRST],
            parameters[FIRST], federates[PUBLISHER], overlappingValue, overlappingTag, firstSourceDesignators,
            "first strict interior-overlap interaction");
         assertDelivery(members[COMBINED], recorders[COMBINED].interactions.get(4),
            interactions[COMBINED], parameters[COMBINED], federates[PUBLISHER], overlappingValue,
            overlappingTag, firstSourceDesignators, "combined strict interior-overlap interaction");

         RegionHandleSet noCommonSubscription = null;
         try {
            DimensionHandleSet noCommonDimensions =
               members[NO_COMMON].getDimensionHandleSetFactory().create();
            noCommonDimensions.add(secondDimensions[NO_COMMON]);
            RegionHandle noCommonSubscriptionRegion = members[NO_COMMON].createRegion(noCommonDimensions);
            checkRegionHandle(noCommonSubscriptionRegion, "no-common subscription region creation");
            regions[NO_COMMON] = new RegionHandle[] {noCommonSubscriptionRegion};
            members[NO_COMMON].setRangeBounds(noCommonSubscriptionRegion,
               secondDimensions[NO_COMMON], new RangeBounds(8, 9));
            noCommonSubscription = regionSet(members[NO_COMMON], regions[NO_COMMON]);
            members[NO_COMMON].commitRegionModifications(noCommonSubscription);
            JavaTckSupport.check(members[NO_COMMON].getDimensionHandleSet(noCommonSubscriptionRegion)
                  .size() == 1
                  && members[NO_COMMON].getDimensionHandleSet(noCommonSubscriptionRegion)
                     .contains(secondDimensions[NO_COMMON]),
               "no-common subscriber region did not contain only Y");
            members[NO_COMMON].subscribeInteractionClassWithRegions(
               interactions[NO_COMMON], noCommonSubscription);
            subscribed[NO_COMMON] = true;

            DimensionHandleSet noCommonSourceDimensions =
               members[PUBLISHER].getDimensionHandleSetFactory().create();
            noCommonSourceDimensions.add(firstDimensions[PUBLISHER]);
            noCommonSource = members[PUBLISHER].createRegion(noCommonSourceDimensions);
            checkRegionHandle(noCommonSource, "no-common source region creation");
            members[PUBLISHER].setRangeBounds(noCommonSource, firstDimensions[PUBLISHER],
               new RangeBounds(8, 9));
            RegionHandleSet noCommonSourceSet = regionSet(members[PUBLISHER], noCommonSource);
            members[PUBLISHER].commitRegionModifications(noCommonSourceSet);
            JavaTckSupport.check(members[PUBLISHER].getDimensionHandleSet(noCommonSource).size() == 1
                  && members[PUBLISHER].getDimensionHandleSet(noCommonSource)
                     .contains(firstDimensions[PUBLISHER])
                  && !firstDimensions[PUBLISHER].equals(secondDimensions[NO_COMMON]),
               "no-common source and subscriber regions unexpectedly shared a dimension");
            byte[] noCommonValue = new byte[] {(byte) 0x91};
            byte[] noCommonTag = new byte[] {0x49, 0x34};
            send(members[PUBLISHER], interactions[PUBLISHER], parameters[PUBLISHER],
               noCommonSourceSet, noCommonValue, noCommonTag);
            await(members, callbackModel, recorders, new int[] {0, 3, 2, 5, 1, 0},
               "no-common-dimension interaction routing");
         } finally {
            if (subscribed[NO_COMMON] && noCommonSubscription != null) try {
               members[NO_COMMON].unsubscribeInteractionClassWithRegions(
                  interactions[NO_COMMON], noCommonSubscription);
               subscribed[NO_COMMON] = false;
            } catch (Exception ignored) { }
            if (noCommonSource != null) try {
               members[PUBLISHER].deleteRegion(noCommonSource);
               noCommonSource = null;
            } catch (Exception ignored) { }
            if (regions[NO_COMMON] != null) try {
               members[NO_COMMON].deleteRegion(regions[NO_COMMON][0]);
               regions[NO_COMMON] = null;
            } catch (Exception ignored) { }
         }
      } finally {
         if (noCommonSource != null && joined[PUBLISHER]) try {
            members[PUBLISHER].deleteRegion(noCommonSource);
         } catch (Exception ignored) { }
         for (int i = FIRST; i < members.length; ++i) {
            if (subscribed[i] && joined[i]) try {
               members[i].unsubscribeInteractionClassWithRegions(interactions[i],
                  regionSet(members[i], regions[i]));
            } catch (Exception ignored) { }
         }
         if (published && joined[PUBLISHER]) try {
            members[PUBLISHER].unpublishInteractionClass(interactions[PUBLISHER]);
         } catch (Exception ignored) { }
         for (int i = 0; i < members.length; ++i) {
            if (joined[i] && regions[i] != null) {
               for (RegionHandle region : regions[i]) try { members[i].deleteRegion(region); }
                  catch (Exception ignored) { }
            }
         }
         for (int i = members.length - 1; i >= 0; --i) {
            if (joined[i]) try { members[i].resignFederationExecution(ResignAction.NO_ACTION); }
               catch (Exception ignored) { }
         }
         if (created) try { members[PUBLISHER].destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         for (int i = members.length - 1; i >= 0; --i) {
            if (connected[i]) try { members[i].disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long firstLower, long firstUpper, long secondLower, long secondUpper)
         throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      checkRegionHandle(region, "multi-region routing region creation");
      ambassador.setRangeBounds(region, first, new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, second, new RangeBounds(secondLower, secondUpper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static void checkRegionHandle(RegionHandle region, String description) {
      JavaTckSupport.check(region != null && region.encodedLength() > 0,
         description + " returned an invalid handle");
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle... regions)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      for (RegionHandle region : regions) result.add(region);
      return result;
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandle parameter, RegionHandleSet regions, byte[] value, byte[] tag)
         throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value.clone());
      publisher.sendInteractionWithRegions(interaction, values, regions, tag.clone());
   }

   private static void assertDelivery(RTIambassador receiver, Interaction actual,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedValue, byte[] expectedTag,
         List<RegionDesignatorSnapshot> expectedRegions, String description) throws Exception {
      Set<RegionDesignatorSnapshot> expected = new HashSet<>(expectedRegions);
      Set<RegionDesignatorSnapshot> actualDesignators = actual.regionDesignators == null
         ? new HashSet<>() : new HashSet<>(actual.regionDesignators);
      JavaTckSupport.check(actual.interaction != null && actual.interaction.encodedLength() > 0
            && actual.interaction.equals(expectedClass)
            && actual.parameters.size() == 1
            && Arrays.equals(expectedValue, actual.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, actual.tag)
            && actual.producer != null && actual.producer.encodedLength() > 0
            && expectedProducer.equals(actual.producer)
            && actual.transportation != null && actual.transportation.encodedLength() > 0
            && actual.regions != null && actual.regions.size() == expectedRegions.size()
            && actual.regionDesignators != null
            && actual.regionDesignators.size() == expectedRegions.size()
            && expected.size() == expectedRegions.size()
            && actualDesignators.size() == actual.regionDesignators.size()
            && expected.equals(actualDesignators),
         description + " changed interaction payload, producer, or source-region set");
      JavaTckSupport.check(!receiver.getTransportationTypeName(actual.transportation).isEmpty(),
         description + " returned an unresolvable transportation type");
   }

   private static List<RegionDesignatorSnapshot> snapshotRegionDesignators(
         RTIambassador ambassador, RegionHandleSet regions, DimensionHandle expectedFirst,
         DimensionHandle expectedSecond) throws Exception {
      List<RegionDesignatorSnapshot> result = new ArrayList<>();
      for (RegionHandle region : regions) {
         checkRegionHandle(region, "source or callback region-designator inspection");
         Set<DimensionHandle> actualDimensions = ambassador.getDimensionHandleSet(region);
         if (expectedFirst != null || expectedSecond != null) {
            JavaTckSupport.check(expectedFirst != null && expectedSecond != null
                  && actualDimensions.size() == 2
                  && actualDimensions.contains(expectedFirst)
                  && actualDimensions.contains(expectedSecond),
               "multi-region interaction region copy did not retain exactly the two routing dimensions");
         }
         Set<String> dimensions = new HashSet<>();
         Map<String, RangeSnapshot> ranges = new HashMap<>();
         for (DimensionHandle dimension : actualDimensions) {
            JavaTckSupport.check(dimension != null && dimension.encodedLength() > 0,
               "multi-region interaction region copy contained an invalid dimension handle");
            String name = ambassador.getDimensionName(dimension);
            RangeBounds bounds = ambassador.getRangeBounds(region, dimension);
            dimensions.add(name);
            ranges.put(name, new RangeSnapshot(bounds.lower, bounds.upper));
         }
         result.add(new RegionDesignatorSnapshot(dimensions, ranges));
      }
      return result;
   }

   private static void await(RTIambassador[] members, CallbackModel model, Recorder[] recorders,
         int[] expectedCounts, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !matches(recorders, expectedCounts)) {
         for (int i = FIRST; i < members.length; ++i) {
            if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(members[i]);
            else Thread.yield();
         }
         Thread.sleep(1L);
      }
      JavaTckSupport.check(matches(recorders, expectedCounts),
         "timed out waiting for " + description + "; observed " + counts(recorders));
   }

   private static boolean matches(Recorder[] recorders, int[] expected) {
      for (int i = 0; i < expected.length; ++i) {
         if (recorders[i].interactions.size() != expected[i]) return false;
      }
      return true;
   }

   private static String counts(Recorder[] recorders) {
      StringBuilder result = new StringBuilder("[");
      for (int i = 0; i < recorders.length; ++i) {
         if (i > 0) result.append(", ");
         result.append(recorders[i].interactions.size());
      }
      return result.append(']').toString();
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK multi-region publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final Set<RegionHandle> regions;
      private final List<RegionDesignatorSnapshot> regionDesignators;

      private Interaction(Object[] arguments, RTIambassador regionInspector,
            DimensionHandle firstDimension, DimensionHandle secondDimension) throws Exception {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
         values.forEach((handle, value) -> {
            JavaTckSupport.check(handle != null && handle.encodedLength() > 0,
               "multi-region callback contained an invalid parameter handle");
            parameters.put(handle, value.clone());
         });
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regions = arguments[5] == null ? null
            : new HashSet<>((RegionHandleSet) arguments[5]);
         regionDesignators = arguments[5] == null ? null
            : snapshotRegionDesignators(regionInspector, (RegionHandleSet) arguments[5],
               firstDimension, secondDimension);
      }
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
      private volatile DimensionHandle firstDimension;
      private volatile DimensionHandle secondDimension;

      private void inspectRegionDesignatorsWith(RTIambassador ambassador,
            DimensionHandle first, DimensionHandle second) {
         regionInspector = ambassador;
         firstDimension = first;
         secondDimension = second;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction(arguments, regionInspector,
                     firstDimension, secondDimension));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK multi-region callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
