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
import hla.rti1516_2025.RangeBounds;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Standard-API region filtering, unsubscription, and resubscription for interactions. */
final class RegionalInteractionRoutingTck {
   private RegionalInteractionRoutingTck() { }

   static void run(RtiFactory factory, String fom, String interactionName,
         String parameterName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      runInternal(factory, fom, interactionName, parameterName, firstDimensionName,
         secondDimensionName, null, callbackModel);
   }

   static void runCustomTransportation(RtiFactory factory, String fom, String interactionName,
         String parameterName, String transportationName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(transportationName != null && !transportationName.isEmpty(),
         "adapter did not supply a custom transportation name");
      runInternal(factory, fom, interactionName, parameterName, firstDimensionName,
         secondDimensionName, transportationName, callbackModel);
   }

   private static void runInternal(RtiFactory factory, String fom, String interactionName,
         String parameterName, String firstDimensionName, String secondDimensionName,
         String expectedTransportationName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador overlap = factory.getRtiAmbassador();
      RTIambassador disjoint = factory.getRtiAmbassador();
      Recorder overlapRecorder = new Recorder();
      Recorder disjointRecorder = new Recorder();
      String federation = "java-tck-regional-interaction-routing-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean overlapConnected = false;
      boolean disjointConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean overlapJoined = false;
      boolean disjointJoined = false;
      boolean published = false;
      boolean overlapSubscribed = false;
      boolean disjointSubscribed = false;
      InteractionClassHandle ownerInteraction = null;
      InteractionClassHandle overlapInteraction = null;
      InteractionClassHandle disjointInteraction = null;
      RegionHandle sourceRegion = null;
      RegionHandle overlapRegion = null;
      RegionHandle disjointRegion = null;
      RegionHandleSet sourceRegions = null;
      RegionHandleSet overlapRegions = null;
      RegionHandleSet disjointRegions = null;
      try {
         owner.connect(noopFederateAmbassador(), callbackModel);
         ownerConnected = true;
         overlap.connect(overlapRecorder.proxy(), callbackModel);
         overlapConnected = true;
         disjoint.connect(disjointRecorder.proxy(), callbackModel);
         disjointConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-region-owner", federation);
         ownerJoined = true;
         overlap.joinFederationExecution("java-tck-region-overlap", federation);
         overlapJoined = true;
         disjoint.joinFederationExecution("java-tck-region-disjoint", federation);
         disjointJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         overlapInteraction = overlap.getInteractionClassHandle(interactionName);
         disjointInteraction = disjoint.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle overlapParameter =
            overlap.getParameterHandle(overlapInteraction, parameterName);
         ParameterHandle disjointParameter =
            disjoint.getParameterHandle(disjointInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle overlapFirst = overlap.getDimensionHandle(firstDimensionName);
         DimensionHandle overlapSecond = overlap.getDimensionHandle(secondDimensionName);
         DimensionHandle disjointFirst = disjoint.getDimensionHandle(firstDimensionName);
         DimensionHandle disjointSecond = disjoint.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(ownerInteraction.equals(overlapInteraction)
               && ownerInteraction.equals(disjointInteraction)
               && ownerParameter.equals(overlapParameter)
               && ownerParameter.equals(disjointParameter)
               && ownerFirst.equals(overlapFirst) && ownerFirst.equals(disjointFirst)
               && ownerSecond.equals(overlapSecond) && ownerSecond.equals(disjointSecond),
            "adapter FOM returned inconsistent interaction, parameter, or dimension handles");
         JavaTckSupport.check(owner.getAvailableDimensionsForInteractionClass(ownerInteraction)
               .contains(ownerFirst)
               && owner.getAvailableDimensionsForInteractionClass(ownerInteraction)
                  .contains(ownerSecond)
               && overlap.getAvailableDimensionsForInteractionClass(overlapInteraction)
                  .contains(overlapFirst)
               && overlap.getAvailableDimensionsForInteractionClass(overlapInteraction)
                  .contains(overlapSecond)
               && disjoint.getAvailableDimensionsForInteractionClass(disjointInteraction)
                  .contains(disjointFirst)
               && disjoint.getAvailableDimensionsForInteractionClass(disjointInteraction)
                  .contains(disjointSecond),
            "adapter interaction class does not expose both configured dimensions at every member");
         JavaTckSupport.check(owner.getDimensionUpperBound(ownerFirst) >= 9
               && owner.getDimensionUpperBound(ownerSecond) >= 9
               && overlap.getDimensionUpperBound(overlapFirst) >= 9
               && overlap.getDimensionUpperBound(overlapSecond) >= 9
               && disjoint.getDimensionUpperBound(disjointFirst) >= 9
               && disjoint.getDimensionUpperBound(disjointSecond) >= 9,
            "adapter interaction dimensions do not contain the routing test ranges");

         owner.publishInteractionClass(ownerInteraction);
         published = true;
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 1, 5);
         overlapRegion = createRegion(overlap, overlapFirst, overlapSecond, 2, 6, 2, 6);
         disjointRegion = createRegion(disjoint, disjointFirst, disjointSecond, 7, 9, 7, 9);
         sourceRegions = regionSet(owner, sourceRegion);
         overlapRegions = regionSet(overlap, overlapRegion);
         disjointRegions = regionSet(disjoint, disjointRegion);
         overlap.setConveyRegionDesignatorSetsSwitch(true);
         disjoint.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(overlap.getConveyRegionDesignatorSetsSwitch()
               && disjoint.getConveyRegionDesignatorSetsSwitch(),
            "convey-region-designator-sets switch did not turn on for both receivers");
         overlap.subscribeInteractionClassWithRegions(overlapInteraction, overlapRegions);
         overlapSubscribed = true;
         disjoint.subscribeInteractionClassWithRegions(disjointInteraction, disjointRegions);
         disjointSubscribed = true;

         byte[] firstValue = new byte[] {0x61, 0x62, 0x63};
         byte[] firstTag = new byte[] {0x49, 0x31};
         send(owner, ownerInteraction, ownerParameter, sourceRegions, firstValue, firstTag);
         await(callbackModel, overlap, () -> overlapRecorder.interactions.size() == 1,
            "ordinary interaction delivered to overlapping region subscription");
         Interaction first = overlapRecorder.interactions.get(0);
         assertDelivery(overlap, first, overlapInteraction, overlapParameter, producer,
            firstValue, firstTag, sourceRegion, expectedTransportationName);
         pump(disjoint, callbackModel);
         JavaTckSupport.check(disjointRecorder.interactions.isEmpty(),
            "disjoint regional interaction subscription received the interaction");

         overlapRecorder.interactions.clear();
         overlap.unsubscribeInteractionClassWithRegions(overlapInteraction, overlapRegions);
         overlapSubscribed = false;
         send(owner, ownerInteraction, ownerParameter, sourceRegions, firstValue, firstTag);
         pump(overlap, callbackModel);
         JavaTckSupport.check(overlapRecorder.interactions.isEmpty(),
            "unsubscribed regional interaction receiver received a message");

         overlap.subscribeInteractionClassWithRegions(overlapInteraction, overlapRegions);
         overlapSubscribed = true;
         byte[] secondValue = new byte[] {0x71, 0x72};
         byte[] secondTag = new byte[] {0x49, 0x32};
         send(owner, ownerInteraction, ownerParameter, sourceRegions, secondValue, secondTag);
         await(callbackModel, overlap, () -> overlapRecorder.interactions.size() == 1,
            "regional interaction delivered after resubscription");
         Interaction second = overlapRecorder.interactions.get(0);
         assertDelivery(overlap, second, overlapInteraction, overlapParameter, producer,
            secondValue, secondTag, sourceRegion, expectedTransportationName);
         pump(disjoint, callbackModel);
         JavaTckSupport.check(disjointRecorder.interactions.isEmpty(),
            "disjoint receiver received a later regional interaction");
      } finally {
         if (overlapSubscribed && overlapJoined) {
            try { overlap.unsubscribeInteractionClassWithRegions(overlapInteraction, overlapRegions); }
            catch (Exception ignored) { }
         }
         if (disjointSubscribed && disjointJoined) {
            try { disjoint.unsubscribeInteractionClassWithRegions(disjointInteraction, disjointRegions); }
            catch (Exception ignored) { }
         }
         if (published && ownerJoined) {
            try { owner.unpublishInteractionClass(ownerInteraction); } catch (Exception ignored) { }
         }
         if (sourceRegion != null && ownerJoined) {
            try { owner.deleteRegion(sourceRegion); } catch (Exception ignored) { }
         }
         if (overlapRegion != null && overlapJoined) {
            try { overlap.deleteRegion(overlapRegion); } catch (Exception ignored) { }
         }
         if (disjointRegion != null && disjointJoined) {
            try { disjoint.deleteRegion(disjointRegion); } catch (Exception ignored) { }
         }
         if (disjointJoined) resign(disjoint);
         if (overlapJoined) resign(overlap);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (disjointConnected) disconnect(disjoint);
         if (overlapConnected) disconnect(overlap);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void send(RTIambassador ambassador, InteractionClassHandle interaction,
         ParameterHandle parameter, RegionHandleSet regions, byte[] value, byte[] tag)
         throws Exception {
      ParameterHandleValueMap parameters =
         ambassador.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value.clone());
      ambassador.sendInteractionWithRegions(interaction, parameters, regions, tag.clone());
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long firstLower, long firstUpper,
         long secondLower, long secondUpper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, second, new RangeBounds(secondLower, secondUpper));
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

   private static void assertDelivery(RTIambassador receiver, Interaction actual,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedValue, byte[] expectedTag,
         RegionHandle expectedRegion, String expectedTransportationName) throws Exception {
      JavaTckSupport.check(actual.interaction.equals(expectedClass)
            && actual.parameters.size() == 1
            && Arrays.equals(expectedValue, actual.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, actual.tag)
            && expectedProducer.equals(actual.producer)
            && actual.transportation != null
            && actual.regions != null && actual.regions.size() == 1
            && actual.regions.contains(expectedRegion),
         "regional interaction did not preserve standard payload and callback metadata");
      String actualTransportationName = receiver.getTransportationTypeName(actual.transportation);
      JavaTckSupport.check(!actualTransportationName.isEmpty()
            && (expectedTransportationName == null
               || expectedTransportationName.equals(actualTransportationName)),
         "regional interaction returned an unresolvable transportation type");
   }

   private static void await(CallbackModel model, RTIambassador ambassador,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(ambassador, model);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         for (int i = 0; i < 8; ++i) ambassador.evokeCallback(0.0);
      } else {
         Thread.yield();
      }
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK regional owner callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final Set<RegionHandle> regions;

      private Interaction(Object[] args) {
         interaction = (InteractionClassHandle) args[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) args[1];
         values.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) args[2]).clone();
         transportation = (TransportationTypeHandle) args[3];
         producer = (FederateHandle) args[4];
         regions = args[5] == null ? null : new HashSet<>((RegionHandleSet) args[5]);
      }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               if ("receiveInteraction".equals(method.getName())
                     && args != null && args.length == 6) {
                  interactions.add(new Interaction(args));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK regional callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
