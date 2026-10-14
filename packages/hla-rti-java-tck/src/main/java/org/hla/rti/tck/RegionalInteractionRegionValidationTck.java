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

/** Checks atomic rejection of regional interaction sends and subscriptions. */
final class RegionalInteractionRegionValidationTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] VALUE = new byte[] {0x01};

   private RegionalInteractionRegionValidationTck() { }

   static void run(RtiFactory factory, String fom, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, String unrelatedDimensionName,
         CallbackModel callbackModel) throws Exception {
      runInvalidSend(factory, fom, interactionName, parameterName, firstDimensionName,
         secondDimensionName, unrelatedDimensionName, callbackModel);
      runInvalidSubscription(factory, fom, interactionName, parameterName, firstDimensionName,
         secondDimensionName, unrelatedDimensionName, callbackModel);
   }

   private static void runInvalidSend(RtiFactory factory, String fom, String interactionName,
         String parameterName, String firstDimensionName, String secondDimensionName,
         String unrelatedDimensionName, CallbackModel callbackModel) throws Exception {
      Scenario scenario = new Scenario(factory, fom, interactionName, parameterName,
         firstDimensionName, secondDimensionName, unrelatedDimensionName, callbackModel,
         "send");
      try {
         scenario.initialize();
         scenario.subscriber.subscribeInteractionClass(scenario.subscriberInteraction);
         scenario.ordinarySubscribed = true;
         scenario.subscriber.setConveyRegionDesignatorSetsSwitch(true);
         ParameterHandleValueMap parameters = scenario.parameters();
         JavaTckSupport.expectFailure(() -> scenario.publisher.sendInteractionWithRegions(
               scenario.publisherInteraction, parameters,
               scenario.regionSet(scenario.publisher, scenario.sourceRegion,
                  scenario.publisherUnrelatedRegion), bytes("mixed")),
            "InvalidRegionContext", "sending with a region on an unavailable dimension");
         pumpSeveral(scenario.subscriber, callbackModel);
         JavaTckSupport.check(scenario.callbacks.interactions.isEmpty(),
            "rejected mixed-context interaction send left a partial delivery behind");

         byte[] validTag = bytes("valid");
         scenario.send(parameters, scenario.regionSet(scenario.publisher, scenario.sourceRegion),
            validTag);
         await(scenario.subscriber, callbackModel,
            () -> scenario.callbacks.interactions.size() >= 1,
            "valid regional interaction after rejected mixed-region send");
         JavaTckSupport.check(scenario.callbacks.interactions.size() == 1,
            "valid interaction after rejected mixed-region send duplicated delivery");
         assertInteraction(scenario.subscriber, scenario.callbacks.interactions.get(0),
            scenario.subscriberInteraction, scenario.subscriberParameter, scenario.producer,
            scenario.sourceRegion, validTag);
      } finally {
         scenario.close();
      }
   }

   private static void runInvalidSubscription(RtiFactory factory, String fom,
         String interactionName, String parameterName, String firstDimensionName,
         String secondDimensionName, String unrelatedDimensionName, CallbackModel callbackModel)
         throws Exception {
      Scenario scenario = new Scenario(factory, fom, interactionName, parameterName,
         firstDimensionName, secondDimensionName, unrelatedDimensionName, callbackModel,
         "subscription");
      try {
         scenario.initialize();
         scenario.subscriber.setConveyRegionDesignatorSetsSwitch(true);
         ParameterHandleValueMap parameters = scenario.parameters();
         JavaTckSupport.expectFailure(() -> scenario.subscriber.subscribeInteractionClassWithRegions(
               scenario.subscriberInteraction,
               scenario.regionSet(scenario.subscriber, scenario.subscriberUnrelatedRegion)),
            "InvalidRegionContext", "subscribing with a region on an unavailable dimension");
         scenario.send(parameters, scenario.regionSet(scenario.publisher, scenario.sourceRegion),
            bytes("after"));
         pumpSeveral(scenario.subscriber, callbackModel);
         JavaTckSupport.check(scenario.callbacks.interactions.isEmpty(),
            "rejected regional interaction subscription left a partial declaration behind");

         scenario.subscriber.subscribeInteractionClassWithRegions(
            scenario.subscriberInteraction,
            scenario.regionSet(scenario.subscriber, scenario.validSubscriptionRegion));
         scenario.subscribed = true;
         byte[] validTag = bytes("valid-sub");
         scenario.send(parameters, scenario.regionSet(scenario.publisher, scenario.sourceRegion),
            validTag);
         await(scenario.subscriber, callbackModel,
            () -> scenario.callbacks.interactions.size() >= 1,
            "valid regional interaction after rejected regional subscription");
         JavaTckSupport.check(scenario.callbacks.interactions.size() == 1,
            "valid interaction after rejected subscription duplicated delivery");
         assertInteraction(scenario.subscriber, scenario.callbacks.interactions.get(0),
            scenario.subscriberInteraction, scenario.subscriberParameter, scenario.producer,
            scenario.sourceRegion, validTag);
      } finally {
         scenario.close();
      }
   }

   private static byte[] bytes(String text) {
      return text.getBytes(java.nio.charset.StandardCharsets.US_ASCII);
   }

   private static void assertInteraction(RTIambassador receiver, Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, RegionHandle expectedRegion, byte[] expectedTag)
         throws Exception {
      JavaTckSupport.check(expectedClass.equals(interaction.interaction)
            && interaction.parameters.size() == 1
            && Arrays.equals(VALUE, interaction.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, interaction.tag)
            && expectedProducer.equals(interaction.producer),
         "valid interaction after region-context rejection changed class, payload, tag, or producer");
      TransportationTypeHandle transportation = interaction.transportation;
      JavaTckSupport.check(transportation != null
            && !receiver.getTransportationTypeName(transportation).isEmpty(),
         "valid interaction returned an unresolvable transportation type");
      JavaTckSupport.check(interaction.regions != null && interaction.regions.size() == 1
            && interaction.regions.contains(expectedRegion),
         "valid interaction did not convey exactly its source region");
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

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK interaction validation publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static final class Scenario {
      private final RTIambassador publisher;
      private final RTIambassador subscriber;
      private final Recorder callbacks = new Recorder();
      private final String federation;
      private final String fom;
      private final String interactionName;
      private final String parameterName;
      private final String firstDimensionName;
      private final String secondDimensionName;
      private final String unrelatedDimensionName;
      private final CallbackModel callbackModel;
      private boolean publisherConnected, subscriberConnected, created;
      private boolean publisherJoined, subscriberJoined, published, subscribed;
      private boolean ordinarySubscribed;
      private InteractionClassHandle publisherInteraction, subscriberInteraction;
      private ParameterHandle publisherParameter, subscriberParameter;
      private DimensionHandle publisherFirst, publisherSecond, publisherUnrelated;
      private DimensionHandle subscriberFirst, subscriberSecond, subscriberUnrelated;
      private RegionHandle sourceRegion, publisherUnrelatedRegion;
      private RegionHandle validSubscriptionRegion, subscriberUnrelatedRegion;
      private FederateHandle producer;

      private Scenario(RtiFactory factory, String fom, String interactionName,
            String parameterName, String firstDimensionName, String secondDimensionName,
            String unrelatedDimensionName, CallbackModel callbackModel, String suffix)
            throws Exception {
         publisher = factory.getRtiAmbassador();
         subscriber = factory.getRtiAmbassador();
         this.fom = fom;
         this.interactionName = interactionName;
         this.parameterName = parameterName;
         this.firstDimensionName = firstDimensionName;
         this.secondDimensionName = secondDimensionName;
         this.unrelatedDimensionName = unrelatedDimensionName;
         this.callbackModel = callbackModel;
         federation = "java-tck-regional-interaction-region-validation-" + suffix + "-"
            + UUID.randomUUID();
      }

      private void initialize() throws Exception {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         subscriber.connect(callbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         producer = publisher.joinFederationExecution(
            "java-tck-region-validation-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-region-validation-subscriber", federation);
         subscriberJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         subscriberInteraction = subscriber.getInteractionClassHandle(interactionName);
         publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         subscriberParameter = subscriber.getParameterHandle(subscriberInteraction, parameterName);
         publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         publisherUnrelated = publisher.getDimensionHandle(unrelatedDimensionName);
         subscriberFirst = subscriber.getDimensionHandle(firstDimensionName);
         subscriberSecond = subscriber.getDimensionHandle(secondDimensionName);
         subscriberUnrelated = subscriber.getDimensionHandle(unrelatedDimensionName);
         JavaTckSupport.check(publisherInteraction.equals(subscriberInteraction)
               && publisherParameter.equals(subscriberParameter)
               && publisherFirst.equals(subscriberFirst)
               && publisherSecond.equals(subscriberSecond)
               && publisherUnrelated.equals(subscriberUnrelated),
            "adapter DDM FOM returned inconsistent handles across federates");
         Set<DimensionHandle> available =
            subscriber.getAvailableDimensionsForInteractionClass(subscriberInteraction);
         JavaTckSupport.check(available.contains(subscriberFirst)
               && available.contains(subscriberSecond)
               && !available.contains(subscriberUnrelated)
               && !subscriberUnrelated.equals(subscriberFirst)
               && !subscriberUnrelated.equals(subscriberSecond)
               && publisher.getDimensionUpperBound(publisherFirst) >= 2
               && publisher.getDimensionUpperBound(publisherSecond) >= 2
               && publisher.getDimensionUpperBound(publisherUnrelated) >= 2,
            "adapter FOM must provide two interaction dimensions and a distinct unrelated dimension");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond);
         publisherUnrelatedRegion = createSingleDimensionRegion(publisher, publisherUnrelated);
         validSubscriptionRegion = createRegion(subscriber, subscriberFirst, subscriberSecond);
         subscriberUnrelatedRegion = createSingleDimensionRegion(subscriber, subscriberUnrelated);
      }

      private ParameterHandleValueMap parameters() throws Exception {
         ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, VALUE.clone());
         return values;
      }

      private void send(ParameterHandleValueMap values, RegionHandleSet regions, byte[] tag)
            throws Exception {
         publisher.sendInteractionWithRegions(publisherInteraction, values, regions, tag.clone());
      }

      private RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle... handles)
            throws Exception {
         RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
         for (RegionHandle handle : handles) result.add(handle);
         return result;
      }

      private void close() {
         if (subscribed && subscriberJoined) try {
            subscriber.unsubscribeInteractionClassWithRegions(subscriberInteraction,
               regionSet(subscriber, validSubscriptionRegion));
         } catch (Exception ignored) { }
         if (ordinarySubscribed && subscriberJoined) try {
            subscriber.unsubscribeInteractionClass(subscriberInteraction);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         deleteRegion(subscriber, subscriberUnrelatedRegion, subscriberJoined);
         deleteRegion(subscriber, validSubscriptionRegion, subscriberJoined);
         deleteRegion(publisher, publisherUnrelatedRegion, publisherJoined);
         deleteRegion(publisher, sourceRegion, publisherJoined);
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

      private RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
            DimensionHandle second) throws Exception {
         DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
         dimensions.add(first);
         dimensions.add(second);
         RegionHandle region = ambassador.createRegion(dimensions);
         ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(0, 2));
         ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(0, 2));
         ambassador.commitRegionModifications(regionSet(ambassador, region));
         return region;
      }

      private RegionHandle createSingleDimensionRegion(RTIambassador ambassador,
            DimensionHandle dimension) throws Exception {
         DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         RegionHandle region = ambassador.createRegion(dimensions);
         ambassador.setRangeBounds(region, dimension,
            new hla.rti1516_2025.RangeBounds(0, 2));
         ambassador.commitRegionModifications(regionSet(ambassador, region));
         return region;
      }

      private void deleteRegion(RTIambassador ambassador, RegionHandle region, boolean joined) {
         if (joined && region != null) try { ambassador.deleteRegion(region); }
            catch (Exception ignored) { }
      }
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
                  values.forEach((handle, value) ->
                     interaction.parameters.put(handle, value.clone()));
                  interaction.tag = ((byte[]) arguments[2]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[3];
                  interaction.producer = (FederateHandle) arguments[4];
                  interaction.regions = arguments[5] == null ? null
                     : new HashSet<>((RegionHandleSet) arguments[5]);
                  interactions.add(interaction);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional validation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
