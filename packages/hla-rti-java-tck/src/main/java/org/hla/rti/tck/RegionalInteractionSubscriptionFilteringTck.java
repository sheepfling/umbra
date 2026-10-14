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

/** Filters ordinary and regional interaction sends using explicit regional declarations. */
final class RegionalInteractionSubscriptionFilteringTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] VALUE = new byte[] {0x2C, 0x01};
   private static final byte[] TAG = new byte[] {0x7A, 0x19};

   private RegionalInteractionSubscriptionFilteringTck() { }

   static void run(RtiFactory factory, String fom, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, String unrelatedDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-regional-interaction-subscription-filter-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false, published = false;
      boolean ordinarySubscribed = false, regionalSubscribed = false;
      boolean sourceRegionCreated = false, subscriberRegionCreated = false;
      boolean uncommittedRegionCreated = false, subscriberWrongContextCreated = false;
      boolean publisherWrongContextCreated = false;
      InteractionClassHandle publisherInteraction = null, subscriberInteraction = null;
      RegionHandle sourceRegion = null, subscriberRegion = null;
      RegionHandle uncommittedRegion = null, subscriberWrongContextRegion = null;
      RegionHandle publisherWrongContextRegion = null;
      RegionHandleSet sourceRegions = null, subscriberRegions = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-regional-filter-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-regional-filter-subscriber", federation);
         subscriberJoined = true;

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
         DimensionHandle unrelatedDimension =
            subscriber.getDimensionHandle(unrelatedDimensionName);
         JavaTckSupport.check(publisherInteraction.equals(subscriberInteraction)
               && publisherParameter.equals(subscriberParameter)
               && publisherFirst.equals(subscriberFirst)
               && publisherSecond.equals(subscriberSecond),
            "adapter DDM FOM returned inconsistent interaction, parameter, or dimension handles");
         checkDimensions(publisher, publisherInteraction, publisherFirst, publisherSecond);
         checkDimensions(subscriber, subscriberInteraction, subscriberFirst, subscriberSecond);
         Set<DimensionHandle> subscriptionDimensions =
            subscriber.getAvailableDimensionsForInteractionClass(subscriberInteraction);
         JavaTckSupport.check(!unrelatedDimension.equals(subscriberFirst)
               && !unrelatedDimension.equals(subscriberSecond)
               && !subscriptionDimensions.contains(unrelatedDimension)
               && subscriber.getDimensionUpperBound(unrelatedDimension) >= 1,
            "adapter unrelated dimension must be distinct and unavailable to the interaction class");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5);
         sourceRegionCreated = true;
         subscriberRegion = createRegion(subscriber, subscriberFirst, subscriberSecond, 2, 6);
         subscriberRegionCreated = true;
         sourceRegions = regionSet(publisher, sourceRegion);
         subscriberRegions = regionSet(subscriber, subscriberRegion);
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, VALUE.clone());

         subscriber.subscribeInteractionClass(subscriberInteraction);
         ordinarySubscribed = true;
         RegionHandleSet emptyRegions = subscriber.getRegionHandleSetFactory().create();
         subscriber.subscribeInteractionClassWithRegions(subscriberInteraction, emptyRegions);
         subscriber.unsubscribeInteractionClassWithRegions(subscriberInteraction, emptyRegions);
         subscriberCallbacks.interactions.clear();
         publisher.sendInteraction(publisherInteraction, parameters, TAG.clone());
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.interactions.size() >= 1,
            "ordinary interaction through the ordinary subscription");
         JavaTckSupport.check(subscriberCallbacks.interactions.size() == 1,
            "ordinary interaction subscription produced a duplicate callback");
         assertInteraction(subscriber, subscriberCallbacks.interactions.get(0),
            subscriberInteraction, subscriberParameter, producer, sourceRegion, false);

         subscriberCallbacks.interactions.clear();
         publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, publisher.getRegionHandleSetFactory().create(), TAG.clone());
         pumpSeveral(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.interactions.isEmpty(),
            "explicitly empty regional send fell back to the ordinary subscription");

         subscriber.subscribeInteractionClassWithRegions(subscriberInteraction, subscriberRegions);
         regionalSubscribed = true;
         subscriber.unsubscribeInteractionClass(subscriberInteraction);
         ordinarySubscribed = false;
         subscriberCallbacks.interactions.clear();
         publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, sourceRegions, TAG.clone());
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.interactions.size() >= 1,
            "explicit regional subscription delivery without source designators");
         JavaTckSupport.check(subscriberCallbacks.interactions.size() == 1,
            "regional interaction produced a duplicate callback");
         assertInteraction(subscriber, subscriberCallbacks.interactions.get(0),
            subscriberInteraction, subscriberParameter, producer, sourceRegion, false);

         subscriber.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(subscriber.getConveyRegionDesignatorSetsSwitch(),
            "subscriber did not enable source-region designator callbacks");
         subscriberCallbacks.interactions.clear();
         publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, sourceRegions, TAG.clone());
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.interactions.size() >= 1,
            "regional interaction with conveyed source designator");
         assertInteraction(subscriber, subscriberCallbacks.interactions.get(0),
            subscriberInteraction, subscriberParameter, producer, sourceRegion, true);

         subscriberCallbacks.interactions.clear();
         publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, sourceRegions, TAG.clone());
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            moveRegion(subscriber, subscriberRegion, subscriberFirst, subscriberSecond, 7, 9);
            pumpSeveral(subscriber, callbackModel);
            JavaTckSupport.check(subscriberCallbacks.interactions.isEmpty(),
               "evoked regional interaction ignored the moved subscription region");
         } else {
            await(subscriber, callbackModel,
               () -> subscriberCallbacks.interactions.size() >= 1,
               "immediate regional interaction before region movement");
            assertInteraction(subscriber, subscriberCallbacks.interactions.get(0),
               subscriberInteraction, subscriberParameter, producer, sourceRegion, true);
            subscriberCallbacks.interactions.clear();
            moveRegion(subscriber, subscriberRegion, subscriberFirst, subscriberSecond, 7, 9);
            pumpSeveral(subscriber, callbackModel);
            JavaTckSupport.check(subscriberCallbacks.interactions.isEmpty(),
               "immediate regional interaction replayed after region movement");
         }

         moveRegion(subscriber, subscriberRegion, subscriberFirst, subscriberSecond, 2, 6);
         subscriberCallbacks.interactions.clear();
         publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, sourceRegions, TAG.clone());
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.interactions.size() >= 1,
            "regional interaction after subscription-region restoration");
         assertInteraction(subscriber, subscriberCallbacks.interactions.get(0),
            subscriberInteraction, subscriberParameter, producer, sourceRegion, true);

         InteractionClassHandle publisherInteractionHandle = publisherInteraction;
         InteractionClassHandle subscriberInteractionHandle = subscriberInteraction;
         RegionHandleSet publisherRegionSet = sourceRegions;
         RegionHandleSet subscriberRegionSet = subscriberRegions;
         JavaTckSupport.expectFailure(() -> publisher.sendInteractionWithRegions(
               publisherInteractionHandle, parameters, subscriberRegionSet, TAG.clone()),
            "RegionNotCreatedByThisFederate", "sending with the subscriber's region");
         JavaTckSupport.expectFailure(() -> subscriber.subscribeInteractionClassWithRegions(
               subscriberInteractionHandle, publisherRegionSet),
            "RegionNotCreatedByThisFederate", "subscribing with the publisher's region");

         DimensionHandleSet subscriberDimensions =
            subscriber.getDimensionHandleSetFactory().create();
         subscriberDimensions.add(subscriberFirst);
         subscriberDimensions.add(subscriberSecond);
         uncommittedRegion = subscriber.createRegion(subscriberDimensions);
         uncommittedRegionCreated = true;
         RegionHandle uncommitted = uncommittedRegion;
         JavaTckSupport.expectFailure(() -> subscriber.subscribeInteractionClassWithRegions(
               subscriberInteractionHandle, regionSet(subscriber, uncommitted)),
            "InvalidRegion", "subscribing with an uncommitted region");

         subscriberWrongContextRegion = createUnrelatedRegion(
            subscriber, unrelatedDimension);
         subscriberWrongContextCreated = true;
         RegionHandle wrongSubscriberRegion = subscriberWrongContextRegion;
         JavaTckSupport.expectFailure(() -> subscriber.subscribeInteractionClassWithRegions(
               subscriberInteractionHandle, regionSet(subscriber, wrongSubscriberRegion)),
            "InvalidRegionContext", "subscribing with an incompatible region context");

         publisherWrongContextRegion = createUnrelatedRegion(
            publisher, publisher.getDimensionHandle(unrelatedDimensionName));
         publisherWrongContextCreated = true;
         RegionHandle wrongPublisherRegion = publisherWrongContextRegion;
         JavaTckSupport.expectFailure(() -> publisher.sendInteractionWithRegions(
               publisherInteractionHandle, parameters, regionSet(publisher, wrongPublisherRegion),
               TAG.clone()),
            "InvalidRegionContext", "sending with an incompatible region context");

         subscriberCallbacks.interactions.clear();
         publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, publisher.getRegionHandleSetFactory().create(),
            TAG.clone());
         pumpSeveral(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.interactions.isEmpty(),
            "empty regional send produced a callback through the regional declaration");

         RegionHandle subscribedRegion = subscriberRegion;
         JavaTckSupport.expectFailure(() -> subscriber.deleteRegion(subscribedRegion),
            "RegionInUseForUpdateOrSubscription",
            "deleting a region still used by the regional interaction subscription");
         subscriber.unsubscribeInteractionClassWithRegions(
            subscriberInteraction, subscriberRegions);
         regionalSubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         subscriber.deleteRegion(subscriberRegion);
         subscriberRegion = null;
         subscriberRegionCreated = false;
         subscriber.deleteRegion(uncommittedRegion);
         uncommittedRegion = null;
         uncommittedRegionCreated = false;
         subscriber.deleteRegion(subscriberWrongContextRegion);
         subscriberWrongContextRegion = null;
         subscriberWrongContextCreated = false;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         sourceRegionCreated = false;
         publisher.deleteRegion(publisherWrongContextRegion);
         publisherWrongContextRegion = null;
         publisherWrongContextCreated = false;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (regionalSubscribed && subscriberJoined && subscriberRegions != null) try {
            subscriber.unsubscribeInteractionClassWithRegions(
               subscriberInteraction, subscriberRegions);
         } catch (Exception ignored) { }
         if (ordinarySubscribed && subscriberJoined && subscriberInteraction != null) try {
            subscriber.unsubscribeInteractionClass(subscriberInteraction);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         deleteRegion(subscriber, subscriberRegion, subscriberRegionCreated, subscriberJoined);
         deleteRegion(subscriber, uncommittedRegion, uncommittedRegionCreated, subscriberJoined);
         deleteRegion(subscriber, subscriberWrongContextRegion,
            subscriberWrongContextCreated, subscriberJoined);
         deleteRegion(publisher, sourceRegion, sourceRegionCreated, publisherJoined);
         deleteRegion(publisher, publisherWrongContextRegion,
            publisherWrongContextCreated, publisherJoined);
         if (subscriberJoined) try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = regionSet(ambassador, region);
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static RegionHandle createUnrelatedRegion(RTIambassador ambassador,
         DimensionHandle unrelatedDimension) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(unrelatedDimension);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, unrelatedDimension,
         new hla.rti1516_2025.RangeBounds(0, 1));
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

   private static void moveRegion(RTIambassador ambassador, RegionHandle region,
         DimensionHandle first, DimensionHandle second, long lower, long upper) throws Exception {
      ambassador.setRangeBounds(region, first,
         new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second,
         new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = regionSet(ambassador, region);
      ambassador.commitRegionModifications(regions);
   }

   private static void checkDimensions(RTIambassador ambassador,
         InteractionClassHandle interaction, DimensionHandle first, DimensionHandle second)
         throws Exception {
      Set<DimensionHandle> available =
         ambassador.getAvailableDimensionsForInteractionClass(interaction);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         "adapter interaction FOM does not provide dimensions/bounds for regional filtering");
   }

   private static void assertInteraction(RTIambassador receiver, Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, RegionHandle sourceRegion, boolean expectRegions)
         throws Exception {
      JavaTckSupport.check(expectedClass.equals(interaction.interaction)
            && interaction.parameters.size() == 1
            && Arrays.equals(VALUE, interaction.parameters.get(expectedParameter))
            && Arrays.equals(TAG, interaction.tag)
            && expectedProducer.equals(interaction.producer),
         "regional interaction filtering changed class, value, tag, or producer metadata");
      TransportationTypeHandle transportation = interaction.transportation;
      JavaTckSupport.check(transportation != null
            && !receiver.getTransportationTypeName(transportation).isEmpty(),
         "regional interaction filtering returned an unresolvable transportation type");
      if (expectRegions) {
         JavaTckSupport.check(interaction.regions != null && interaction.regions.size() == 1
               && interaction.regions.contains(sourceRegion),
            "regional interaction filtering omitted its source-region designator");
      } else {
         JavaTckSupport.check(interaction.regions == null,
            "regional interaction filtering unexpectedly conveyed a source region");
      }
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
         boolean created, boolean joined) {
      if (created && joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK regional filter publisher";
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
                  if ("toString".equals(name)) return "Java TCK regional interaction filtering callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
