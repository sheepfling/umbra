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

/** Preserves regional interaction routing through active/passive transitions. */
final class PassiveRegionalInteractionTransitionTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] VALUE = new byte[] {0x2A, 0x15};

   private PassiveRegionalInteractionTransitionTck() { }

   static void run(RtiFactory factory, String fom, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador active = factory.getRtiAmbassador();
      RTIambassador passive = factory.getRtiAmbassador();
      Recorder activeCallbacks = new Recorder();
      Recorder passiveCallbacks = new Recorder();
      String federation = "java-tck-passive-regional-interaction-transition-" + UUID.randomUUID();
      boolean publisherConnected = false, activeConnected = false, passiveConnected = false;
      boolean created = false, publisherJoined = false, activeJoined = false, passiveJoined = false;
      boolean published = false, activeSubscribed = false, passiveActive = false;
      boolean passiveSubscribed = false;
      RegionHandle sourceRegion = null, activeRegion = null, passiveRegion = null;
      InteractionClassHandle publisherInteraction = null, activeInteraction = null;
      InteractionClassHandle passiveInteraction = null;
      RegionHandleSet activeRegions = null, passiveRegions = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         active.connect(activeCallbacks.proxy(), callbackModel);
         activeConnected = true;
         passive.connect(passiveCallbacks.proxy(), callbackModel);
         passiveConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producingFederate = publisher.joinFederationExecution(
            "java-tck-passive-interaction-publisher", federation);
         publisherJoined = true;
         active.joinFederationExecution("java-tck-passive-interaction-active", federation);
         activeJoined = true;
         passive.joinFederationExecution("java-tck-passive-interaction-passive", federation);
         passiveJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         activeInteraction = active.getInteractionClassHandle(interactionName);
         passiveInteraction = passive.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle activeParameter = active.getParameterHandle(activeInteraction, parameterName);
         ParameterHandle passiveParameter =
            passive.getParameterHandle(passiveInteraction, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle activeFirst = active.getDimensionHandle(firstDimensionName);
         DimensionHandle activeSecond = active.getDimensionHandle(secondDimensionName);
         DimensionHandle passiveFirst = passive.getDimensionHandle(firstDimensionName);
         DimensionHandle passiveSecond = passive.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherInteraction.equals(activeInteraction)
               && publisherInteraction.equals(passiveInteraction)
               && publisherParameter.equals(activeParameter)
               && publisherParameter.equals(passiveParameter)
               && publisherFirst.equals(activeFirst) && publisherSecond.equals(activeSecond)
               && publisherFirst.equals(passiveFirst) && publisherSecond.equals(passiveSecond),
            "adapter DDM FOM handles changed across passive interaction federates");
         checkDimensions(publisher, publisherInteraction, publisherFirst, publisherSecond);
         checkDimensions(active, activeInteraction, activeFirst, activeSecond);
         checkDimensions(passive, passiveInteraction, passiveFirst, passiveSecond);

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5);
         activeRegion = createRegion(active, activeFirst, activeSecond, 2, 6);
         passiveRegion = createRegion(passive, passiveFirst, passiveSecond, 2, 6);
         activeRegions = regionSet(active, activeRegion);
         passiveRegions = regionSet(passive, passiveRegion);
         active.setConveyRegionDesignatorSetsSwitch(true);
         passive.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(active.getConveyRegionDesignatorSetsSwitch()
               && passive.getConveyRegionDesignatorSetsSwitch(),
            "receivers did not enable region designator callbacks");

         active.subscribeInteractionClassWithRegions(activeInteraction, activeRegions);
         activeSubscribed = true;
         passive.subscribeInteractionClassWithRegions(passiveInteraction, passiveRegions);
         passiveActive = true;

         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, VALUE.clone());
         byte[] firstTag = new byte[] {0x70, 0x31};
         send(publisher, publisherInteraction, parameters,
            regionSet(publisher, sourceRegion), firstTag);
         await(active, passive, callbackModel,
            () -> activeCallbacks.interactions.size() >= 1
               && passiveCallbacks.interactions.size() >= 1,
            "initial delivery to active regional subscribers");
         requireInteraction(active, activeCallbacks.interactions.get(0), activeInteraction,
            activeParameter, producingFederate, sourceRegion, firstTag,
            "active subscriber initial delivery");
         requireInteraction(passive, passiveCallbacks.interactions.get(0), passiveInteraction,
            passiveParameter, producingFederate, sourceRegion, firstTag,
            "passive candidate initial active delivery");
         JavaTckSupport.check(activeCallbacks.interactions.size() == 1
               && passiveCallbacks.interactions.size() == 1,
            "initial regional interaction produced duplicate callbacks");

         activeCallbacks.interactions.clear();
         passiveCallbacks.interactions.clear();
         passive.subscribeInteractionClassPassivelyWithRegions(passiveInteraction, passiveRegions);
         passiveActive = false;
         passiveSubscribed = true;
         byte[] retainedTag = new byte[] {0x70, 0x32};
         send(publisher, publisherInteraction, parameters,
            regionSet(publisher, sourceRegion), retainedTag);
         await(active, passive, callbackModel,
            () -> activeCallbacks.interactions.size() >= 1
               && passiveCallbacks.interactions.size() >= 1,
            "passive route delivery while another active route remains");
         requireInteraction(active, activeCallbacks.interactions.get(0), activeInteraction,
            activeParameter, producingFederate, sourceRegion, retainedTag,
            "active route while passive declaration is retained");
         requireInteraction(passive, passiveCallbacks.interactions.get(0), passiveInteraction,
            passiveParameter, producingFederate, sourceRegion, retainedTag,
            "passive route delivery while active relevance remains");
         JavaTckSupport.check(activeCallbacks.interactions.size() == 1
               && passiveCallbacks.interactions.size() == 1,
            "retained passive declaration produced duplicate callbacks");

         activeCallbacks.interactions.clear();
         passiveCallbacks.interactions.clear();
         active.unsubscribeInteractionClassWithRegions(activeInteraction, activeRegions);
         activeSubscribed = false;
         send(publisher, publisherInteraction, parameters, regionSet(publisher, sourceRegion),
            new byte[] {0x70, 0x33});
         pumpSeveral(active, passive, callbackModel);
         JavaTckSupport.check(activeCallbacks.interactions.isEmpty()
               && passiveCallbacks.interactions.isEmpty(),
            "passive regional interaction delivered without an active route");

         RegionHandleSet emptyRegions = passive.getRegionHandleSetFactory().create();
         passive.subscribeInteractionClassWithRegions(passiveInteraction, emptyRegions);
         send(publisher, publisherInteraction, parameters, regionSet(publisher, sourceRegion),
            new byte[] {0x70, 0x34});
         pumpSeveral(active, passive, callbackModel);
         JavaTckSupport.check(passiveCallbacks.interactions.isEmpty(),
            "empty regional subscription set reactivated a passive declaration");

         passive.subscribeInteractionClassWithRegions(passiveInteraction, passiveRegions);
         passiveActive = true;
         passiveSubscribed = false;
         byte[] restoredTag = new byte[] {0x70, 0x35};
         send(publisher, publisherInteraction, parameters,
            regionSet(publisher, sourceRegion), restoredTag);
         await(active, passive, callbackModel,
            () -> passiveCallbacks.interactions.size() >= 1,
            "delivery after passive declaration is reactivated");
         JavaTckSupport.check(activeCallbacks.interactions.isEmpty(),
            "unsubscribed active regional receiver received a later interaction");
         requireInteraction(passive, passiveCallbacks.interactions.get(0), passiveInteraction,
            passiveParameter, producingFederate, sourceRegion, restoredTag,
            "reactivated regional interaction delivery");
         JavaTckSupport.check(passiveCallbacks.interactions.size() == 1,
            "reactivated regional route produced duplicate callbacks");

         passive.unsubscribeInteractionClassWithRegions(passiveInteraction, passiveRegions);
         passiveActive = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         active.deleteRegion(activeRegion);
         activeRegion = null;
         passive.deleteRegion(passiveRegion);
         passiveRegion = null;
         passive.resignFederationExecution(ResignAction.NO_ACTION);
         passiveJoined = false;
         active.resignFederationExecution(ResignAction.NO_ACTION);
         activeJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (activeSubscribed && activeJoined && activeRegions != null) try {
            active.unsubscribeInteractionClassWithRegions(activeInteraction, activeRegions);
         } catch (Exception ignored) { }
         if ((passiveActive || passiveSubscribed) && passiveJoined && passiveRegions != null) try {
            passive.unsubscribeInteractionClassWithRegions(passiveInteraction, passiveRegions);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         deleteRegion(publisher, sourceRegion, publisherJoined);
         deleteRegion(active, activeRegion, activeJoined);
         deleteRegion(passive, passiveRegion, passiveJoined);
         if (passiveJoined) try { passive.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (activeJoined) try { active.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (activeConnected) try { active.disconnect(); } catch (Exception ignored) { }
         if (passiveConnected) try { passive.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador,
         InteractionClassHandle interaction, DimensionHandle first, DimensionHandle second)
         throws Exception {
      DimensionHandleSet available = ambassador.getAvailableDimensionsForInteractionClass(interaction);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 6
            && ambassador.getDimensionUpperBound(second) >= 6,
         "adapter interaction class must expose both dimensions and adequate bounds");
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

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandleValueMap values, RegionHandleSet regions, byte[] tag) throws Exception {
      publisher.sendInteractionWithRegions(interaction, values, regions, tag.clone());
   }

   private static void requireInteraction(RTIambassador receiver, Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, RegionHandle expectedRegion, byte[] expectedTag,
         String description) throws Exception {
      JavaTckSupport.check(expectedClass.equals(interaction.interaction)
            && interaction.parameters.size() == 1
            && Arrays.equals(VALUE, interaction.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, interaction.tag)
            && expectedProducer.equals(interaction.producer),
         description + " changed interaction class, parameter, tag, or producer");
      JavaTckSupport.check(interaction.transportation != null
            && !receiver.getTransportationTypeName(interaction.transportation).isEmpty(),
         description + " returned an unresolvable transportation handle");
      JavaTckSupport.check(interaction.regions != null && interaction.regions.size() == 1
            && interaction.regions.contains(expectedRegion),
         description + " omitted its source-region designator");
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
               if ("toString".equals(method.getName())) return "Java TCK passive interaction publisher";
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
                  if ("toString".equals(name)) return "Java TCK passive interaction callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
