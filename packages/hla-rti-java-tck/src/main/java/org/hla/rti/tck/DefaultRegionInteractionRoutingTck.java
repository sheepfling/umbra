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

/** Resolves ordinary interaction delivery through explicit and implicit default regions. */
final class DefaultRegionInteractionRoutingTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private DefaultRegionInteractionRoutingTck() { }

   static void run(RtiFactory factory, String fom, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador regional = factory.getRtiAmbassador();
      RTIambassador mixed = factory.getRtiAmbassador();
      Recorder regionalCallbacks = new Recorder();
      Recorder mixedCallbacks = new Recorder();
      String federation = "java-tck-default-region-interaction-routing-" + UUID.randomUUID();
      boolean publisherConnected = false, regionalConnected = false, mixedConnected = false;
      boolean created = false, publisherJoined = false, regionalJoined = false, mixedJoined = false;
      boolean published = false, regionalSubscribed = false, mixedOrdinarySubscribed = false;
      boolean mixedRegionalSubscribed = false;
      RegionHandle sourceRegion = null, regionalRegion = null, mixedRegion = null;
      InteractionClassHandle publisherInteraction = null, regionalInteraction = null;
      InteractionClassHandle mixedInteraction = null;
      ParameterHandle publisherParameter = null, regionalParameter = null, mixedParameter = null;
      RegionHandleSet sourceRegions = null, regionalRegions = null, mixedRegions = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         regional.connect(regionalCallbacks.proxy(), callbackModel);
         regionalConnected = true;
         mixed.connect(mixedCallbacks.proxy(), callbackModel);
         mixedConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producingFederate = publisher.joinFederationExecution(
            "java-tck-default-region-publisher", federation);
         publisherJoined = true;
         regional.joinFederationExecution("java-tck-default-region-regional", federation);
         regionalJoined = true;
         mixed.joinFederationExecution("java-tck-default-region-mixed", federation);
         mixedJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         regionalInteraction = regional.getInteractionClassHandle(interactionName);
         mixedInteraction = mixed.getInteractionClassHandle(interactionName);
         publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         regionalParameter = regional.getParameterHandle(regionalInteraction, parameterName);
         mixedParameter = mixed.getParameterHandle(mixedInteraction, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle regionalFirst = regional.getDimensionHandle(firstDimensionName);
         DimensionHandle regionalSecond = regional.getDimensionHandle(secondDimensionName);
         DimensionHandle mixedFirst = mixed.getDimensionHandle(firstDimensionName);
         DimensionHandle mixedSecond = mixed.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherInteraction.equals(regionalInteraction)
               && publisherInteraction.equals(mixedInteraction)
               && publisherParameter.equals(regionalParameter)
               && publisherParameter.equals(mixedParameter)
               && publisherFirst.equals(regionalFirst) && publisherSecond.equals(regionalSecond)
               && publisherFirst.equals(mixedFirst) && publisherSecond.equals(mixedSecond),
            "adapter DDM FOM handles changed across default-region interaction federates");
         checkDimensions(publisher, publisherInteraction, publisherFirst, publisherSecond);
         checkDimensions(regional, regionalInteraction, regionalFirst, regionalSecond);
         checkDimensions(mixed, mixedInteraction, mixedFirst, mixedSecond);

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 0, 1);
         regionalRegion = createRegion(regional, regionalFirst, regionalSecond, 0, 1);
         mixedRegion = createRegion(mixed, mixedFirst, mixedSecond, 2, 3);
         sourceRegions = regionSet(publisher, sourceRegion);
         regionalRegions = regionSet(regional, regionalRegion);
         mixedRegions = regionSet(mixed, mixedRegion);

         regional.setConveyRegionDesignatorSetsSwitch(true);
         mixed.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(regional.getConveyRegionDesignatorSetsSwitch()
               && mixed.getConveyRegionDesignatorSetsSwitch(),
            "default-region receivers did not enable region designator callbacks");
         regional.subscribeInteractionClassWithRegions(regionalInteraction, regionalRegions);
         regionalSubscribed = true;
         mixed.subscribeInteractionClass(mixedInteraction);
         mixedOrdinarySubscribed = true;
         mixed.subscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         mixedRegionalSubscribed = true;

         byte[] firstValue = new byte[] {(byte) 0xD1, 0x04};
         byte[] firstTag = new byte[] {0x01, 0x02};
         sendRegional(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, firstValue), sourceRegions, firstTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.interactions.size() >= 1,
            "explicit regional send to an overlapping regional subscription");
         assertInteraction(regional, regionalCallbacks.interactions.get(0), regionalInteraction,
            regionalParameter, producingFederate, firstValue, firstTag,
            regionSetMembership(sourceRegion), "overlapping regional delivery");
         pumpSeveral(regional, mixed, callbackModel);
         JavaTckSupport.check(regionalCallbacks.interactions.size() == 1
               && mixedCallbacks.interactions.isEmpty(),
            "disjoint regional subscription received an explicit regional interaction");

         mixed.unsubscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         mixedRegionalSubscribed = false;
         byte[] secondValue = new byte[] {0x71, 0x72};
         byte[] secondTag = new byte[] {0x03, 0x04};
         sendRegional(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, secondValue), sourceRegions, secondTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.interactions.size() >= 2
               && mixedCallbacks.interactions.size() >= 1,
            "ordinary subscription restoring explicit regional delivery");
         JavaTckSupport.check(regionalCallbacks.interactions.size() == 2
               && mixedCallbacks.interactions.size() == 1,
            "ordinary subscription restoration produced an unexpected callback count");
         assertInteraction(regional, regionalCallbacks.interactions.get(1), regionalInteraction,
            regionalParameter, producingFederate, secondValue, secondTag, null,
            "regional receiver after explicit regional send");
         assertInteraction(mixed, mixedCallbacks.interactions.get(0), mixedInteraction,
            mixedParameter, producingFederate, secondValue, secondTag, null,
            "ordinary receiver after explicit regional send");

         mixed.subscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         mixedRegionalSubscribed = true;
         byte[] thirdValue = new byte[] {0x05, 0x06};
         byte[] thirdTag = new byte[] {0x07, 0x08};
         sendOrdinary(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, thirdValue), thirdTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.interactions.size() >= 3
               && mixedCallbacks.interactions.size() >= 2,
            "ordinary send through the implicit default region");
         JavaTckSupport.check(regionalCallbacks.interactions.size() == 3
               && mixedCallbacks.interactions.size() == 2,
            "ordinary default-region send produced an unexpected callback count");
         assertInteraction(regional, regionalCallbacks.interactions.get(2), regionalInteraction,
            regionalParameter, producingFederate, thirdValue, thirdTag,
            java.util.Collections.emptySet(), "regional receiver after ordinary send");
         assertInteraction(mixed, mixedCallbacks.interactions.get(1), mixedInteraction,
            mixedParameter, producingFederate, thirdValue, thirdTag,
            java.util.Collections.emptySet(), "mixed receiver after ordinary send");

         mixed.unsubscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         mixedRegionalSubscribed = false;
         mixed.unsubscribeInteractionClass(mixedInteraction);
         mixedOrdinarySubscribed = false;
         regional.unsubscribeInteractionClassWithRegions(regionalInteraction, regionalRegions);
         regionalSubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         regional.deleteRegion(regionalRegion);
         regionalRegion = null;
         mixed.deleteRegion(mixedRegion);
         mixedRegion = null;
         mixed.resignFederationExecution(ResignAction.NO_ACTION);
         mixedJoined = false;
         regional.resignFederationExecution(ResignAction.NO_ACTION);
         regionalJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (mixedRegionalSubscribed && mixedJoined && mixedRegions != null) try {
            mixed.unsubscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         } catch (Exception ignored) { }
         if (mixedOrdinarySubscribed && mixedJoined && mixedInteraction != null) try {
            mixed.unsubscribeInteractionClass(mixedInteraction);
         } catch (Exception ignored) { }
         if (regionalSubscribed && regionalJoined && regionalRegions != null) try {
            regional.unsubscribeInteractionClassWithRegions(regionalInteraction, regionalRegions);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         deleteRegion(publisher, sourceRegion, publisherJoined);
         deleteRegion(regional, regionalRegion, regionalJoined);
         deleteRegion(mixed, mixedRegion, mixedJoined);
         if (mixedJoined) try { mixed.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (regionalJoined) try { regional.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (regionalConnected) try { regional.disconnect(); } catch (Exception ignored) { }
         if (mixedConnected) try { mixed.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador,
         InteractionClassHandle interaction, DimensionHandle first, DimensionHandle second)
         throws Exception {
      DimensionHandleSet available = ambassador.getAvailableDimensionsForInteractionClass(interaction);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 3
            && ambassador.getDimensionUpperBound(second) >= 3,
         "adapter interaction class must expose both dimensions and sufficient range");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      return regions;
   }

   private static ParameterHandleValueMap parameterMap(RTIambassador ambassador,
         ParameterHandle parameter, byte[] value) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value.clone());
      return values;
   }

   private static void sendRegional(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandleValueMap parameters, RegionHandleSet regions, byte[] tag) throws Exception {
      publisher.sendInteractionWithRegions(interaction, parameters, regions, tag.clone());
   }

   private static void sendOrdinary(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandleValueMap parameters, byte[] tag) throws Exception {
      publisher.sendInteraction(interaction, parameters, tag.clone());
   }

   private static Set<RegionHandle> regionSetMembership(RegionHandle region) {
      Set<RegionHandle> result = new HashSet<>();
      result.add(region);
      return result;
   }

   private static void assertInteraction(RTIambassador receiver, Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedValue, byte[] expectedTag,
         Set<RegionHandle> expectedRegions, String description) throws Exception {
      JavaTckSupport.check(expectedClass.equals(interaction.interaction)
            && interaction.parameters.size() == 1
            && Arrays.equals(expectedValue, interaction.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, interaction.tag)
            && expectedProducer.equals(interaction.producer),
         description + " changed class, parameter, payload, tag, or producer");
      JavaTckSupport.check(interaction.transportation != null
            && !receiver.getTransportationTypeName(interaction.transportation).isEmpty(),
         description + " returned an unresolvable transportation type");
      JavaTckSupport.check(interaction.regions != null
            && (expectedRegions == null || interaction.regions.equals(expectedRegions)),
         description + " returned incorrect source-region metadata");
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         pump(second, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpSeveral(RTIambassador first, RTIambassador second, CallbackModel model)
         throws Exception {
      for (int pass = 0; pass < 8; ++pass) {
         pump(first, model);
         pump(second, model);
      }
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
               if ("toString".equals(method.getName())) return "Java TCK default-region publisher";
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
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

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
                  interactions.add(interaction);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK default-region callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
