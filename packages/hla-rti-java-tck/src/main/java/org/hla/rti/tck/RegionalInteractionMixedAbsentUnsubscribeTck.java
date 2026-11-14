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
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Preserves an active interaction region when unsubscription also names an absent region. */
final class RegionalInteractionMixedAbsentUnsubscribeTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] VALUE = new byte[] {0x52};

   private RegionalInteractionMixedAbsentUnsubscribeTck() { }

   static void run(RtiFactory factory, String fom, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-regional-interaction-mixed-absent-unsubscribe-"
         + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false, published = false;
      boolean subscribed = false;
      InteractionClassHandle publisherInteraction = null, subscriberInteraction = null;
      RegionHandle sourceRegion = null, activeRegion = null, absentRegion = null;
      RegionHandleSet sourceRegions = null, activeRegions = null, mixedRegions = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         subscriber.connect(recorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-mixed-absent-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-mixed-absent-subscriber", federation);
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
         JavaTckSupport.check(publisherInteraction.equals(subscriberInteraction)
               && publisherParameter.equals(subscriberParameter)
               && publisherFirst.equals(subscriberFirst)
               && publisherSecond.equals(subscriberSecond),
            "adapter DDM FOM returned inconsistent interaction, parameter, or dimension handles");
         checkDimensions(publisher, publisherInteraction, publisherFirst, publisherSecond);
         checkDimensions(subscriber, subscriberInteraction, subscriberFirst, subscriberSecond);

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 0, 2);
         activeRegion = createRegion(subscriber, subscriberFirst, subscriberSecond, 0, 2);
         absentRegion = createRegion(subscriber, subscriberFirst, subscriberSecond, 5, 7);
         sourceRegions = regionSet(publisher, sourceRegion);
         activeRegions = regionSet(subscriber, activeRegion);
         mixedRegions = regionSet(subscriber, activeRegion, absentRegion);
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, VALUE.clone());

         subscriber.subscribeInteractionClassWithRegions(subscriberInteraction, activeRegions);
         subscribed = true;
         byte[] initialTag = new byte[] {0x49, 0x4E, 0x49};
         send(publisher, publisherInteraction, parameters, sourceRegions, initialTag);
         await(subscriber, callbackModel, () -> recorder.interactions.size() >= 1,
            "delivery before mixed present/absent unsubscription");
         JavaTckSupport.check(recorder.interactions.size() == 1,
            "initial regional interaction subscription delivered a duplicate callback");
         assertInteraction(recorder.interactions.get(0), subscriberInteraction,
            subscriberParameter, producer, initialTag, "delivery before mixed unsubscription");

         subscriber.unsubscribeInteractionClassWithRegions(subscriberInteraction, mixedRegions);
         byte[] mixedTag = new byte[] {0x4D, 0x49, 0x58};
         send(publisher, publisherInteraction, parameters, sourceRegions, mixedTag);
         await(subscriber, callbackModel, () -> recorder.interactions.size() >= 2,
            "delivery after mixed present/absent unsubscription");
         JavaTckSupport.check(recorder.interactions.size() == 2,
            "unsubscription containing an absent region changed the subscription set");
         assertInteraction(recorder.interactions.get(1), subscriberInteraction,
            subscriberParameter, producer, mixedTag,
            "delivery after mixed present/absent unsubscription");

         subscriber.unsubscribeInteractionClassWithRegions(subscriberInteraction, activeRegions);
         subscribed = false;
         byte[] removedRouteTag = new byte[] {0x52, 0x45, 0x4D};
         send(publisher, publisherInteraction, parameters, sourceRegions, removedRouteTag);
         pumpSeveral(subscriber, callbackModel);
         JavaTckSupport.check(recorder.interactions.size() == 2,
            "mixed absent-region no-op prevented later removal of the active region");

         subscriber.deleteRegion(absentRegion);
         absentRegion = null;
         subscriber.deleteRegion(activeRegion);
         activeRegion = null;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && subscriberJoined && subscriberInteraction != null && activeRegions != null) {
            try { subscriber.unsubscribeInteractionClassWithRegions(subscriberInteraction, activeRegions); }
            catch (Exception ignored) { }
         }
         deleteRegion(subscriber, absentRegion, subscriberJoined);
         deleteRegion(subscriber, activeRegion, subscriberJoined);
         deleteRegion(publisher, sourceRegion, publisherJoined);
         if (published && publisherJoined && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) try {
            subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandleValueMap parameters, RegionHandleSet regions, byte[] tag) throws Exception {
      publisher.sendInteractionWithRegions(interaction, parameters, regions, tag.clone());
   }

   private static void checkDimensions(RTIambassador ambassador,
         InteractionClassHandle interaction, DimensionHandle first, DimensionHandle second)
         throws Exception {
      java.util.Set<DimensionHandle> dimensions =
         ambassador.getAvailableDimensionsForInteractionClass(interaction);
      JavaTckSupport.check(dimensions.contains(first) && dimensions.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 7
            && ambassador.getDimensionUpperBound(second) >= 7,
         "adapter interaction class does not expose the configured DDM dimensions and bounds");
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

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle... regions)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      for (RegionHandle region : regions) result.add(region);
      return result;
   }

   private static void assertInteraction(Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedTag, String description) {
      JavaTckSupport.check(expectedClass.equals(interaction.interaction)
            && interaction.parameters.size() == 1
            && Arrays.equals(VALUE, interaction.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, interaction.tag)
            && expectedProducer.equals(interaction.producer),
         description + " changed the interaction class, parameter, value, tag, or producer");
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
               if ("toString".equals(method.getName())) return "Java TCK mixed-region publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private FederateHandle producer;
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
                  interaction.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  interaction.tag = ((byte[]) arguments[2]).clone();
                  interaction.producer = (FederateHandle) arguments[4];
                  interactions.add(interaction);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK mixed-region interaction callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
