package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Restore an explicit-source regional interaction for two independent recipients. */
final class RestoreRegionalInteractionMultiRecipientTck {
   private RestoreRegionalInteractionMultiRecipientTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel model)
         throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_EVOKED,
         "multi-recipient regional restore has native evidence only for evoked callbacks");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder firstEvents = new Recorder();
      Recorder secondEvents = new Recorder();
      List<RTIambassador> members = List.of(publisher, first, second);
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-restore-regional-interaction-multi-" + UUID.randomUUID();
      String saveLabel = "tck-regional-interaction-multi-baseline";
      byte[] payload = {'M', 'U', 'L', 'T', 'I'};
      byte[] tag = {'R', 'E', 'S', 'T', 'O', 'R', 'E'};
      boolean created = false;
      RegionHandle sourceRegion = null, firstRegion = null, secondRegion = null;
      boolean firstSubscribed = false, secondSubscribed = false;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         connected.add(publisher);
         first.connect(firstEvents.proxy(), model);
         connected.add(first);
         second.connect(secondEvents.proxy(), model);
         connected.add(second);
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "live-tso-multi-publisher", "publisher", federation);
         joined.add(publisher);
         first.joinFederationExecution("live-tso-multi-first", "subscriber", federation);
         joined.add(first);
         second.joinFederationExecution("live-tso-multi-second", "subscriber", federation);
         joined.add(second);

         InteractionClassHandle publisherInteraction = publisher.getInteractionClassHandle(
            interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         InteractionClassHandle firstInteraction = first.getInteractionClassHandle(
            interactionClassName);
         ParameterHandle firstParameter = first.getParameterHandle(
            firstInteraction, parameterName);
         InteractionClassHandle secondInteraction = second.getInteractionClassHandle(
            interactionClassName);
         ParameterHandle secondParameter = second.getParameterHandle(
            secondInteraction, parameterName);
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload.clone());
         publisher.publishInteractionClass(publisherInteraction);
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         sourceRegion = createRegion(publisher, firstDimensionName, secondDimensionName, 1, 7);
         firstRegion = createRegion(first, firstDimensionName, secondDimensionName, 3, 8);
         secondRegion = createRegion(second, firstDimensionName, secondDimensionName, 5, 9);
         first.subscribeInteractionClassWithRegions(firstInteraction, regionSet(first, firstRegion));
         firstSubscribed = true;
         second.subscribeInteractionClassWithRegions(secondInteraction, regionSet(second, secondRegion));
         secondSubscribed = true;
         JavaTckSupport.check(!first.getConveyRegionDesignatorSetsSwitch()
               && !second.getConveyRegionDesignatorSetsSwitch(),
            "default conveyed-region switches were not false");
         first.setConveyRegionDesignatorSetsSwitch(true);
         second.setConveyRegionDesignatorSetsSwitch(true);
         first.enableTimeConstrained();
         second.enableTimeConstrained();
         first.evokeCallback(0.0);
         second.evokeCallback(0.0);
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time1 = offset(initial, epsilon, 1);
         LogicalTime<?, ?> time5 = offset(initial, epsilon, 5);
         LogicalTime<?, ?> time7 = offset(initial, epsilon, 7);
         LogicalTime<?, ?> time10 = offset(initial, epsilon, 10);
         publisher.enableTimeRegulation(interval(epsilon, 5));
         await(members, () -> publisherEvents.regulationEnabled == 1,
            "publisher time-regulation enablement");

         MessageRetractionReturn saved = publisher.sendInteractionWithRegions(
            publisherInteraction, values, regionSet(publisher, sourceRegion), tag.clone(), time7);
         JavaTckSupport.check(saved != null && saved.retractionHandleIsValid
               && saved.handle != null && firstEvents.deliveries.isEmpty()
               && secondEvents.deliveries.isEmpty(),
            "timestamp-seven interaction was not queued for both recipients");
         first.timeAdvanceRequest(time1);
         second.timeAdvanceRequest(time1);
         publisher.requestFederationSave(saveLabel);
         await(members, () -> publisherEvents.saveLabels.equals(List.of(saveLabel))
               && firstEvents.saveLabels.equals(List.of(saveLabel))
               && secondEvents.saveLabels.equals(List.of(saveLabel))
               && firstEvents.timeGrants.size() == 1
               && secondEvents.timeGrants.size() == 1,
            "three-member save initiation and time-one grants");
         JavaTckSupport.check(firstEvents.timeGrants.get(0).equals(time1)
               && secondEvents.timeGrants.get(0).equals(time1),
            "a constrained recipient did not reach time one before save");
         publisher.federateSaveBegun();
         first.federateSaveBegun();
         second.federateSaveBegun();
         publisher.federateSaveComplete();
         first.federateSaveComplete();
         second.federateSaveComplete();
         await(members, () -> publisherEvents.savedCount == 1
               && firstEvents.savedCount == 1 && secondEvents.savedCount == 1,
            "three-member federation saved");
         JavaTckSupport.check(firstEvents.deliveries.isEmpty()
               && secondEvents.deliveries.isEmpty(),
            "a queued interaction arrived while saving");

         publisher.retract(saved.handle);
         first.evokeCallback(0.0);
         second.evokeCallback(0.0);
         JavaTckSupport.check(firstEvents.deliveries.isEmpty()
               && secondEvents.deliveries.isEmpty(),
            "post-save retract leaked a queued interaction");
         JavaTckSupport.expectFailure(() -> publisher.retract(saved.handle),
            "MessageCanNoLongerBeRetracted", "post-save terminal handle");
         publisher.requestFederationRestore(saveLabel);
         await(members, () -> publisherEvents.restoreLabels.equals(List.of(saveLabel))
               && firstEvents.restoreLabels.equals(List.of(saveLabel))
               && secondEvents.restoreLabels.equals(List.of(saveLabel)),
            "three-member restore initiation");
         publisher.federateRestoreComplete();
         first.federateRestoreComplete();
         second.federateRestoreComplete();
         await(members, () -> publisherEvents.restoredCount == 1
               && firstEvents.restoredCount == 1 && secondEvents.restoredCount == 1,
            "three-member federation restored");

         firstEvents.order.clear();
         secondEvents.order.clear();
         first.flushQueueRequest(time10);
         await(members, () -> firstEvents.deliveries.size() == 1
               && firstEvents.flushGrants.size() == 1,
            "first restored interaction and flush queue grant");
         JavaTckSupport.check(secondEvents.deliveries.isEmpty()
               && secondEvents.flushGrants.isEmpty() && secondEvents.order.isEmpty(),
            "first FQR prematurely released second recipient");
         second.flushQueueRequest(time10);
         await(members, () -> secondEvents.deliveries.size() == 1
               && secondEvents.flushGrants.size() == 1,
            "second restored interaction and flush queue grant");
         checkDelivered(firstEvents, firstInteraction, firstParameter, sourceRegion,
            publisherHandle, saved.handle, time7, time5, payload, tag);
         checkDelivered(secondEvents, secondInteraction, secondParameter, sourceRegion,
            publisherHandle, saved.handle, time7, time5, payload, tag);

         firstEvents.order.clear();
         secondEvents.order.clear();
         publisher.retract(saved.handle);
         await(members, () -> firstEvents.retractions.size() == 1
               && secondEvents.retractions.size() == 1,
            "both restored request-retraction callbacks");
         JavaTckSupport.check(firstEvents.retractions.get(0).equals(saved.handle)
               && secondEvents.retractions.get(0).equals(saved.handle)
               && firstEvents.order.equals(List.of("request-retraction"))
               && secondEvents.order.equals(List.of("request-retraction")),
            "restored retraction callbacks differed between recipients");
         JavaTckSupport.expectFailure(() -> publisher.retract(saved.handle),
            "MessageCanNoLongerBeRetracted", "restored terminal handle");

         first.unsubscribeInteractionClassWithRegions(firstInteraction, regionSet(first, firstRegion));
         firstSubscribed = false;
         second.unsubscribeInteractionClassWithRegions(secondInteraction, regionSet(second, secondRegion));
         secondSubscribed = false;
         first.deleteRegion(firstRegion);
         firstRegion = null;
         second.deleteRegion(secondRegion);
         secondRegion = null;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         first.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(first);
         second.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(second);
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(publisher);
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (firstSubscribed) try { first.unsubscribeInteractionClassWithRegions(
            first.getInteractionClassHandle(interactionClassName), regionSet(first, firstRegion)); }
            catch (Exception ignored) { }
         if (secondSubscribed) try { second.unsubscribeInteractionClassWithRegions(
            second.getInteractionClassHandle(interactionClassName), regionSet(second, secondRegion)); }
            catch (Exception ignored) { }
         if (firstRegion != null) try { first.deleteRegion(firstRegion); }
            catch (Exception ignored) { }
         if (secondRegion != null) try { second.deleteRegion(secondRegion); }
            catch (Exception ignored) { }
         if (sourceRegion != null) try { publisher.deleteRegion(sourceRegion); }
            catch (Exception ignored) { }
         for (int index = joined.size() - 1; index >= 0; --index)
            try { joined.get(index).resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         for (int index = connected.size() - 1; index >= 0; --index)
            try { connected.get(index).disconnect(); } catch (Exception ignored) { }
      }
   }

   private static RegionHandle createRegion(RTIambassador member,
         String firstName, String secondName, int lower, int upper) throws Exception {
      DimensionHandle first = member.getDimensionHandle(firstName);
      DimensionHandle second = member.getDimensionHandle(secondName);
      JavaTckSupport.check(member.getDimensionUpperBound(first) >= 9
            && member.getDimensionUpperBound(second) >= 9,
         "adapter DDM FOM dimensions do not support the test ranges");
      DimensionHandleSet dimensions = member.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = member.createRegion(dimensions);
      member.setRangeBounds(region, first, new RangeBounds(lower, upper));
      member.setRangeBounds(region, second, new RangeBounds(lower, upper));
      member.commitRegionModifications(regionSet(member, region));
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador member, RegionHandle region)
         throws Exception {
      RegionHandleSet regions = member.getRegionHandleSetFactory().create();
      regions.add(region);
      return regions;
   }

   private static void checkDelivered(Recorder events,
         InteractionClassHandle interaction, ParameterHandle parameter,
         RegionHandle sourceRegion,
         FederateHandle publisher, MessageRetractionHandle saved,
         LogicalTime<?, ?> time7, LogicalTime<?, ?> time5,
         byte[] payload, byte[] tag) {
      Delivery delivered = events.deliveries.get(0);
      FlushGrant grant = events.flushGrants.get(0);
      JavaTckSupport.check(delivered.interaction.equals(interaction)
            && delivered.parameters.size() == 1
            && Arrays.equals(delivered.parameters.get(parameter), payload)
            && Arrays.equals(delivered.tag, tag)
            && delivered.producer.equals(publisher)
            && delivered.time.equals(time7)
            && delivered.sentOrder == OrderType.TIMESTAMP
            && delivered.receivedOrder == OrderType.TIMESTAMP
            && delivered.regions != null && delivered.regions.size() == 1
            && delivered.regions.contains(sourceRegion)
            && delivered.retraction.equals(saved)
            && grant.time.equals(time5) && grant.optimisticTime.equals(time7)
            && events.order.equals(List.of("interaction", "flush-grant")),
         "restored recipient interaction, grant, or callback ordering changed");
   }

   private static void await(List<RTIambassador> members,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline)
         for (RTIambassador member : members) member.evokeCallback(0.0);
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval interval(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int index = 1; index < steps; ++index)
         result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static final class Delivery {
      InteractionClassHandle interaction;
      ParameterHandleValueMap parameters;
      byte[] tag;
      FederateHandle producer;
      RegionHandleSet regions;
      LogicalTime<?, ?> time;
      OrderType sentOrder, receivedOrder;
      MessageRetractionHandle retraction;
   }

   private static final class FlushGrant {
      LogicalTime<?, ?> time, optimisticTime;
   }

   private static final class Recorder {
      final List<Delivery> deliveries = new CopyOnWriteArrayList<>();
      final List<FlushGrant> flushGrants = new CopyOnWriteArrayList<>();
      final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      final List<LogicalTime<?, ?>> timeGrants = new CopyOnWriteArrayList<>();
      final List<String> saveLabels = new CopyOnWriteArrayList<>();
      final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      final List<String> order = new CopyOnWriteArrayList<>();
      volatile int regulationEnabled, savedCount, restoredCount;

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("timeAdvanceGrant".equals(name))
                  timeGrants.add((LogicalTime<?, ?>) args[0]);
               else if ("initiateFederateSave".equals(name)) saveLabels.add((String) args[0]);
               else if ("federationSaved".equals(name)) savedCount++;
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) restoredCount++;
               else if ("receiveInteraction".equals(name) && args.length == 10) {
                  Delivery delivered = new Delivery();
                  delivered.interaction = (InteractionClassHandle) args[0];
                  delivered.parameters = ((ParameterHandleValueMap) args[1]).clone();
                  delivered.tag = ((byte[]) args[2]).clone();
                  delivered.producer = (FederateHandle) args[4];
                  delivered.regions = args[5] == null ? null :
                     (RegionHandleSet) ((RegionHandleSet) args[5]).clone();
                  delivered.time = (LogicalTime<?, ?>) args[6];
                  delivered.sentOrder = (OrderType) args[7];
                  delivered.receivedOrder = (OrderType) args[8];
                  delivered.retraction = (MessageRetractionHandle) args[9];
                  deliveries.add(delivered);
                  order.add("interaction");
               } else if ("flushQueueGrant".equals(name)) {
                  FlushGrant grant = new FlushGrant();
                  grant.time = (LogicalTime<?, ?>) args[0];
                  grant.optimisticTime = (LogicalTime<?, ?>) args[1];
                  flushGrants.add(grant);
                  order.add("flush-grant");
               } else if ("requestRetraction".equals(name)) {
                  retractions.add((MessageRetractionHandle) args[0]);
                  order.add("request-retraction");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional multi-restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
