package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
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

   private MultiRegionInteractionRoutingTck() { }

   static void run(RtiFactory factory, String fom, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador[] members = new RTIambassador[5];
      Recorder[] recorders = new Recorder[5];
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
         FederateHandle producer = members[PUBLISHER].joinFederationExecution(
            "java-tck-multi-region-publisher", federation);
         joined[PUBLISHER] = true;
         for (int i = FIRST; i < members.length; ++i) {
            members[i].joinFederationExecution("java-tck-multi-region-member-" + i, federation);
            joined[i] = true;
         }

         DimensionHandle[] firstDimensions = new DimensionHandle[members.length];
         DimensionHandle[] secondDimensions = new DimensionHandle[members.length];
         for (int i = 0; i < members.length; ++i) {
            interactions[i] = members[i].getInteractionClassHandle(interactionName);
            parameters[i] = members[i].getParameterHandle(interactions[i], parameterName);
            firstDimensions[i] = members[i].getDimensionHandle(firstDimensionName);
            secondDimensions[i] = members[i].getDimensionHandle(secondDimensionName);
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
                  && parameters[PUBLISHER].equals(parameters[i]),
               "multi-region federates resolved different interaction or parameter handles");
         }
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

         for (int i = FIRST; i < members.length; ++i) {
            members[i].setConveyRegionDesignatorSetsSwitch(true);
            JavaTckSupport.check(members[i].getConveyRegionDesignatorSetsSwitch(),
               "multi-region receiver did not enable source-region designators");
            members[i].subscribeInteractionClassWithRegions(interactions[i],
               regionSet(members[i], regions[i]));
            subscribed[i] = true;
         }

         RegionHandleSet bothSources = regionSet(members[PUBLISHER], regions[PUBLISHER]);
         RegionHandleSet firstSource = regionSet(members[PUBLISHER], regions[PUBLISHER][0]);
         RegionHandleSet secondSource = regionSet(members[PUBLISHER], regions[PUBLISHER][1]);
         byte[] bothValue = new byte[] {0x61, 0x62, 0x63};
         byte[] bothTag = new byte[] {0x49, 0x31};
         send(members[PUBLISHER], interactions[PUBLISHER], parameters[PUBLISHER],
            bothSources, bothValue, bothTag);
         await(members, callbackModel, recorders, new int[] {0, 1, 1, 1, 0},
            "multi-region interaction union delivery");
         assertDelivery(members[FIRST], recorders[FIRST].interactions.get(0), interactions[FIRST],
            parameters[FIRST], producer, bothValue, bothTag, regions[PUBLISHER], "first route");
         assertDelivery(members[SECOND], recorders[SECOND].interactions.get(0), interactions[SECOND],
            parameters[SECOND], producer, bothValue, bothTag, regions[PUBLISHER], "second route");
         assertDelivery(members[COMBINED], recorders[COMBINED].interactions.get(0),
            interactions[COMBINED], parameters[COMBINED], producer, bothValue, bothTag,
            regions[PUBLISHER], "combined route without duplicate delivery");

         byte[] secondValue = new byte[] {0x71, 0x72};
         byte[] secondTag = new byte[] {0x49, 0x32};
         send(members[PUBLISHER], interactions[PUBLISHER], parameters[PUBLISHER],
            secondSource, secondValue, secondTag);
         await(members, callbackModel, recorders, new int[] {0, 1, 2, 2, 0},
            "second-source interaction delivery");
         assertDelivery(members[SECOND], recorders[SECOND].interactions.get(1), interactions[SECOND],
            parameters[SECOND], producer, secondValue, secondTag, regions[PUBLISHER],
            "second-source route");
         assertDelivery(members[COMBINED], recorders[COMBINED].interactions.get(1),
            interactions[COMBINED], parameters[COMBINED], producer, secondValue, secondTag,
            regions[PUBLISHER], "combined second-source route");

         byte[] firstValue = new byte[] {(byte) 0x81, (byte) 0x82};
         byte[] firstTag = new byte[] {0x49, 0x33};
         send(members[PUBLISHER], interactions[PUBLISHER], parameters[PUBLISHER],
            firstSource, firstValue, firstTag);
         await(members, callbackModel, recorders, new int[] {0, 2, 2, 3, 0},
            "first-source interaction delivery");
         assertDelivery(members[FIRST], recorders[FIRST].interactions.get(1), interactions[FIRST],
            parameters[FIRST], producer, firstValue, firstTag, regions[PUBLISHER],
            "first-source route");
         assertDelivery(members[COMBINED], recorders[COMBINED].interactions.get(2),
            interactions[COMBINED], parameters[COMBINED], producer, firstValue, firstTag,
            regions[PUBLISHER], "combined first-source route");
      } finally {
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
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(secondLower, secondUpper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
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
         RegionHandle[] expectedRegions, String description) throws Exception {
      Set<RegionHandle> expected = new HashSet<>(Arrays.asList(expectedRegions));
      JavaTckSupport.check(actual.interaction.equals(expectedClass)
            && actual.parameters.size() == 1
            && Arrays.equals(expectedValue, actual.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, actual.tag)
            && expectedProducer.equals(actual.producer)
            && actual.transportation != null
            && expected.equals(actual.regions),
         description + " changed interaction payload, producer, or source-region set");
      JavaTckSupport.check(!receiver.getTransportationTypeName(actual.transportation).isEmpty(),
         description + " returned an unresolvable transportation type");
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

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
         values.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regions = arguments[5] == null ? null
            : new HashSet<>((RegionHandleSet) arguments[5]);
      }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) interactions.add(new Interaction(arguments));
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
