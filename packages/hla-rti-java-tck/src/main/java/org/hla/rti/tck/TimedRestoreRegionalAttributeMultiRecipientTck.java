package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
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

/** Evoked-only parity for restored, timed explicit-source regional attribute fanout. */
final class TimedRestoreRegionalAttributeMultiRecipientTck {
   private TimedRestoreRegionalAttributeMultiRecipientTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel model) throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_EVOKED,
         "timed regional attribute restore has native evidence only for evoked callbacks");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder ownerEvents = new Recorder(publisher);
      Recorder firstEvents = new Recorder(first);
      Recorder secondEvents = new Recorder(second);
      List<RTIambassador> members = List.of(publisher, first, second);
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-timed-regional-attribute-multi-" + UUID.randomUUID();
      String saveLabel = "tck-timed-regional-attribute-multi-" + UUID.randomUUID();
      byte[] value = {'M', 'U', 'L', 'T', 'I'};
      byte[] tag = {'R', 'E', 'S', 'T', 'O', 'R', 'E'};
      boolean created = false;
      RegionHandle sourceRegion = null;
      AttributeSetRegionSetPairList sourcePair = null;
      RegionSubscription firstSubscription = null, secondSubscription = null;
      try {
         publisher.connect(ownerEvents.proxy(), model);
         connected.add(publisher);
         first.connect(firstEvents.proxy(), model);
         connected.add(first);
         second.connect(secondEvents.proxy(), model);
         connected.add(second);
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "timed-regional-owner", "publisher", federation);
         joined.add(publisher);
         first.joinFederationExecution("timed-regional-first", "subscriber", federation);
         joined.add(first);
         second.joinFederationExecution("timed-regional-second", "subscriber", federation);
         joined.add(second);

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         AttributeHandleSet published = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         publisher.changeDefaultAttributeOrderType(publisherClass, published, OrderType.TIMESTAMP);
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 7);
         sourcePair = association(publisher, publisherAttribute, sourceRegion);
         firstSubscription = subscribe(first, objectClassName, attributeName,
            firstDimensionName, secondDimensionName, 3, 8);
         secondSubscription = subscribe(second, objectClassName, attributeName,
            firstDimensionName, secondDimensionName, 5, 9);
         firstEvents.firstDimension = firstSubscription.firstDimension;
         firstEvents.secondDimension = firstSubscription.secondDimension;
         secondEvents.firstDimension = secondSubscription.firstDimension;
         secondEvents.secondDimension = secondSubscription.secondDimension;
         ObjectInstanceHandle object = publisher.registerObjectInstanceWithRegions(
            publisherClass, sourcePair);
         JavaTckSupport.check(object != null, "regional object registration returned no handle");
         await(members, () -> firstEvents.discoveries.contains(object)
               && secondEvents.discoveries.contains(object),
            "regional object discovery at both recipients");

         first.enableTimeConstrained();
         second.enableTimeConstrained();
         await(members, () -> firstEvents.constrainedEnabled == 1
               && secondEvents.constrainedEnabled == 1,
            "both recipients' time-constrained enablement");
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time6 = offset(initial, epsilon, 6);
         LogicalTime<?, ?> time7 = offset(initial, epsilon, 7);
         LogicalTime<?, ?> time8 = offset(initial, epsilon, 8);
         LogicalTime<?, ?> time10 = offset(initial, epsilon, 10);
         publisher.enableTimeRegulation(epsilon);
         await(members, () -> ownerEvents.regulationEnabled == 1,
            "publisher time-regulation enablement");

         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value.clone());
         MessageRetractionReturn saved = publisher.updateAttributeValues(
            object, values, tag.clone(), time8);
         JavaTckSupport.check(saved != null && saved.retractionHandleIsValid
               && saved.handle != null && firstEvents.reflections.isEmpty()
               && secondEvents.reflections.isEmpty(),
            "timestamp-eight regional update was not queued for both recipients");
         publisher.requestFederationSave(saveLabel, time6);
         first.timeAdvanceRequest(time6);
         second.timeAdvanceRequest(time6);
         publisher.timeAdvanceRequest(time6);
         await(members, () -> ownerEvents.saveLabels.equals(List.of(saveLabel))
               && firstEvents.saveLabels.equals(List.of(saveLabel))
               && secondEvents.saveLabels.equals(List.of(saveLabel))
               && ownerEvents.saveTimes.equals(List.of(time6))
               && firstEvents.saveTimes.equals(List.of(time6))
               && secondEvents.saveTimes.equals(List.of(time6))
               && ownerEvents.timeGrants.size() == 1
               && firstEvents.timeGrants.size() == 1
               && secondEvents.timeGrants.size() == 1,
            "timed save initiation and time-six grants");
         for (Recorder events : List.of(ownerEvents, firstEvents, secondEvents))
            JavaTckSupport.check(events.timeGrants.get(0).equals(time6),
               "federate did not reach the timed save boundary");
         for (RTIambassador member : members) {
            member.federateSaveBegun();
            member.federateSaveComplete();
         }
         await(members, () -> ownerEvents.savedCount == 1
               && firstEvents.savedCount == 1 && secondEvents.savedCount == 1,
            "timed regional federation saved");
         JavaTckSupport.check(firstEvents.reflections.isEmpty()
               && secondEvents.reflections.isEmpty(),
            "queued regional update arrived at the save boundary");

         publisher.retract(saved.handle);
         first.evokeCallback(0.0);
         second.evokeCallback(0.0);
         JavaTckSupport.check(firstEvents.reflections.isEmpty()
               && secondEvents.reflections.isEmpty(),
            "post-save retract leaked a queued regional reflection");
         JavaTckSupport.expectFailure(() -> publisher.retract(saved.handle),
            "MessageCanNoLongerBeRetracted", "post-save regional update handle");
         publisher.unassociateRegionsForUpdates(object, sourcePair);
         publisher.requestFederationRestore(saveLabel);
         await(members, () -> ownerEvents.restoreLabels.equals(List.of(saveLabel))
               && firstEvents.restoreLabels.equals(List.of(saveLabel))
               && secondEvents.restoreLabels.equals(List.of(saveLabel)),
            "timed regional restore initiation");
         for (RTIambassador member : members) member.federateRestoreComplete();
         await(members, () -> ownerEvents.restoredCount == 1
               && firstEvents.restoredCount == 1 && secondEvents.restoredCount == 1,
            "timed regional federation restored");

         firstEvents.order.clear();
         secondEvents.order.clear();
         firstEvents.snapshotEnabled = true;
         secondEvents.snapshotEnabled = true;
         first.flushQueueRequest(time10);
         await(List.of(first), () -> firstEvents.reflections.size() == 1
               && firstEvents.flushGrants.size() == 1,
            "first restored regional reflection and FQG");
         JavaTckSupport.check(secondEvents.reflections.isEmpty()
               && secondEvents.flushGrants.isEmpty() && secondEvents.order.isEmpty(),
            "first FQR prematurely released the second regional reflection");
         second.flushQueueRequest(time10);
         await(List.of(second), () -> secondEvents.reflections.size() == 1
               && secondEvents.flushGrants.size() == 1,
            "second restored regional reflection and FQG");
         checkReflected(firstEvents, firstSubscription, object, publisherHandle,
            saved.handle, time8, time7, value, tag);
         checkReflected(secondEvents, secondSubscription, object, publisherHandle,
            saved.handle, time8, time7, value, tag);
         firstEvents.snapshotEnabled = false;
         secondEvents.snapshotEnabled = false;

         firstEvents.order.clear();
         secondEvents.order.clear();
         publisher.retract(saved.handle);
         await(members, () -> firstEvents.retractions.size() == 1
               && secondEvents.retractions.size() == 1,
            "both restored regional request-retraction callbacks");
         JavaTckSupport.check(firstEvents.retractions.get(0).equals(saved.handle)
               && secondEvents.retractions.get(0).equals(saved.handle)
               && firstEvents.order.equals(List.of("request-retraction"))
               && secondEvents.order.equals(List.of("request-retraction")),
            "restored regional retraction callbacks changed their handles or order");
         JavaTckSupport.expectFailure(() -> publisher.retract(saved.handle),
            "MessageCanNoLongerBeRetracted", "restored regional update handle");
         publisher.unassociateRegionsForUpdates(object, sourcePair);
         unsubscribe(first, firstSubscription);
         firstSubscription = null;
         unsubscribe(second, secondSubscription);
         secondSubscription = null;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         for (int index = joined.size() - 1; index >= 0; --index)
            joined.get(index).resignFederationExecution(ResignAction.NO_ACTION);
         joined.clear();
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (firstSubscription != null) try { unsubscribe(first, firstSubscription); }
            catch (Exception ignored) { }
         if (secondSubscription != null) try { unsubscribe(second, secondSubscription); }
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

   private static RegionSubscription subscribe(RTIambassador member,
         String className, String attributeName, String firstName, String secondName,
         int lower, int upper) throws Exception {
      RegionSubscription result = new RegionSubscription();
      result.objectClass = member.getObjectClassHandle(className);
      result.attribute = member.getAttributeHandle(result.objectClass, attributeName);
      result.firstDimension = member.getDimensionHandle(firstName);
      result.secondDimension = member.getDimensionHandle(secondName);
      result.region = createRegion(member, result.firstDimension, result.secondDimension,
         lower, upper);
      result.pair = association(member, result.attribute, result.region);
      member.subscribeObjectClassAttributesWithRegions(result.objectClass, result.pair);
      JavaTckSupport.check(!member.getConveyRegionDesignatorSetsSwitch(),
         "default conveyed-region switch was not false");
      member.setConveyRegionDesignatorSetsSwitch(true);
      return result;
   }

   private static RegionHandle createRegion(RTIambassador member,
         DimensionHandle first, DimensionHandle second, int lower, int upper) throws Exception {
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

   private static AttributeSetRegionSetPairList association(RTIambassador member,
         AttributeHandle attribute, RegionHandle region) throws Exception {
      AttributeSetRegionSetPairList pair = member.getAttributeSetRegionSetPairListFactory().create(1);
      pair.add(new AttributeRegionAssociation(
         JavaTckSupport.attributeSet(member, attribute), regionSet(member, region)));
      return pair;
   }

   private static void unsubscribe(RTIambassador member, RegionSubscription subscription)
         throws Exception {
      member.unsubscribeObjectClassAttributesWithRegions(
         subscription.objectClass, subscription.pair);
      member.deleteRegion(subscription.region);
   }

   private static void checkReflected(Recorder events, RegionSubscription subscription,
         ObjectInstanceHandle object, FederateHandle publisher,
         MessageRetractionHandle saved, LogicalTime<?, ?> time8, LogicalTime<?, ?> time7,
         byte[] value, byte[] tag) {
      JavaTckSupport.check(events.reflections.size() == 1 && events.flushGrants.size() == 1,
         "recipient did not receive exactly one restored reflection and FQG");
      Reflection reflected = events.reflections.get(0);
      FlushGrant grant = events.flushGrants.get(0);
      JavaTckSupport.check(reflected.object.equals(object)
            && reflected.values.size() == 1
            && Arrays.equals(reflected.values.get(subscription.attribute), value)
            && Arrays.equals(reflected.tag, tag)
            && reflected.producer.equals(publisher)
            && reflected.time.equals(time8)
            && reflected.sentOrder == OrderType.TIMESTAMP
            && reflected.receivedOrder == OrderType.TIMESTAMP
            && reflected.regions != null && reflected.regions.size() == 1
            && saved.equals(reflected.retraction),
         "restored regional reflection changed its source, timestamp, order, or payload");
      if (events.snapshotFailure != null)
         throw new AssertionError("callback-time source-region query failed", events.snapshotFailure);
      JavaTckSupport.check(events.snapshotQueried
            && events.snapshotDimensions != null
            && events.snapshotDimensions.size() == 2
            && events.snapshotDimensions.contains(subscription.firstDimension)
            && events.snapshotDimensions.contains(subscription.secondDimension)
            && new RangeBounds(1, 7).equals(events.snapshotFirstBounds)
            && new RangeBounds(1, 7).equals(events.snapshotSecondBounds),
         "receiver-visible copied source region lost its saved dimensions or bounds");
      JavaTckSupport.check(grant.time.equals(time7)
            && grant.optimisticTime.equals(time8)
            && events.order.equals(List.of("reflect", "flush-grant")),
         "restored regional reflection did not precede the time-seven FQG");
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

   private static final class RegionSubscription {
      ObjectClassHandle objectClass;
      AttributeHandle attribute;
      DimensionHandle firstDimension, secondDimension;
      RegionHandle region;
      AttributeSetRegionSetPairList pair;
   }

   private static final class Reflection {
      ObjectInstanceHandle object;
      AttributeHandleValueMap values;
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
      final RTIambassador ambassador;
      final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      final List<FlushGrant> flushGrants = new CopyOnWriteArrayList<>();
      final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      final List<LogicalTime<?, ?>> timeGrants = new CopyOnWriteArrayList<>();
      final List<String> saveLabels = new CopyOnWriteArrayList<>();
      final List<LogicalTime<?, ?>> saveTimes = new CopyOnWriteArrayList<>();
      final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      final List<String> order = new CopyOnWriteArrayList<>();
      volatile int regulationEnabled, constrainedEnabled, savedCount, restoredCount;
      volatile boolean snapshotEnabled, snapshotQueried;
      volatile DimensionHandle firstDimension, secondDimension;
      volatile DimensionHandleSet snapshotDimensions;
      volatile RangeBounds snapshotFirstBounds, snapshotSecondBounds;
      volatile Throwable snapshotFailure;

      Recorder(RTIambassador ambassador) { this.ambassador = ambassador; }

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name))
                  discoveries.add((ObjectInstanceHandle) args[0]);
               else if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("timeConstrainedEnabled".equals(name)) constrainedEnabled++;
               else if ("timeAdvanceGrant".equals(name))
                  timeGrants.add((LogicalTime<?, ?>) args[0]);
               else if ("initiateFederateSave".equals(name)) {
                  saveLabels.add((String) args[0]);
                  if (args.length == 2) saveTimes.add((LogicalTime<?, ?>) args[1]);
               }
               else if ("federationSaved".equals(name)) savedCount++;
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) restoredCount++;
               else if ("reflectAttributeValues".equals(name) && args.length == 10) {
                  Reflection reflected = new Reflection();
                  reflected.object = (ObjectInstanceHandle) args[0];
                  reflected.values = ((AttributeHandleValueMap) args[1]).clone();
                  reflected.tag = ((byte[]) args[2]).clone();
                  reflected.producer = (FederateHandle) args[4];
                  reflected.regions = args[5] == null ? null :
                     (RegionHandleSet) ((RegionHandleSet) args[5]).clone();
                  reflected.time = (LogicalTime<?, ?>) args[6];
                  reflected.sentOrder = (OrderType) args[7];
                  reflected.receivedOrder = (OrderType) args[8];
                  reflected.retraction = (MessageRetractionHandle) args[9];
                  if (snapshotEnabled) {
                     try {
                        JavaTckSupport.check(reflected.regions != null
                              && reflected.regions.size() == 1,
                           "timed reflection did not convey one copied source region");
                        RegionHandle copied = reflected.regions.iterator().next();
                        snapshotDimensions = ambassador.getDimensionHandleSet(copied);
                        JavaTckSupport.check(snapshotDimensions.size() == 2
                              && snapshotDimensions.contains(firstDimension)
                              && snapshotDimensions.contains(secondDimension),
                           "copied source region did not retain receiver DDM dimensions");
                        snapshotFirstBounds = ambassador.getRangeBounds(copied, firstDimension);
                        snapshotSecondBounds = ambassador.getRangeBounds(copied, secondDimension);
                        snapshotQueried = true;
                     } catch (Throwable failure) {
                        snapshotFailure = failure;
                     }
                  }
                  reflections.add(reflected);
                  order.add("reflect");
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
                  if ("toString".equals(name)) return "Java TCK timed regional attribute restore";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
