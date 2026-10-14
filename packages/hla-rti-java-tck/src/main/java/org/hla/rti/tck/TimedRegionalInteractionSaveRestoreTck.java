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
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Preserves a queued regional TSO interaction and retraction state across federation restore. */
final class TimedRegionalInteractionSaveRestoreTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private TimedRegionalInteractionSaveRestoreTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      runInternal(factory, fom, timeImplementation, interactionClassName, parameterName,
         firstDimensionName, secondDimensionName, callbackModel, false);
   }

   static void runDefaultRegion(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      runInternal(factory, fom, timeImplementation, interactionClassName, parameterName,
         firstDimensionName, secondDimensionName, callbackModel, true);
   }

   private static void runInternal(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel, boolean defaultRegion)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String caseName = defaultRegion ? "timed-default-region-interaction" : "timed-regional-interaction";
      String federation = "java-tck-" + caseName + "-save-restore-" + UUID.randomUUID();
      String label = "java-tck-" + caseName + "-save-restore";
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean regulating = false, constrained = false, published = false, subscribed = false;
      InteractionClassHandle publisherInteraction = null, receiverInteraction = null;
      RegionHandle sourceRegion = null, receiverRegion = null;
      RegionHandleSet sourceRegions = null, receiverRegions = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-timed-regional-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-timed-regional-receiver", federation);
         receiverJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter)
               && publisherFirst.equals(receiverFirst) && publisherSecond.equals(receiverSecond)
               && publisher.getAvailableDimensionsForInteractionClass(publisherInteraction)
                  .contains(publisherFirst)
               && publisher.getAvailableDimensionsForInteractionClass(publisherInteraction)
                  .contains(publisherSecond)
               && publisher.getDimensionUpperBound(publisherFirst) >= 6
               && publisher.getDimensionUpperBound(publisherSecond) >= 6,
            "adapter interaction FOM handles or dimensional metadata were inconsistent");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         if (!defaultRegion) {
            sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5, 1, 5);
            sourceRegions = regionSet(publisher, sourceRegion);
         }
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6, 2, 6);
         receiverRegions = regionSet(receiver, receiverRegion);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "timed regional restore did not enable source-region callback designators");
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "timed regional members selected different logical-time factories");
         LogicalTimeInterval publisherEpsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon = (LogicalTimeInterval) receiverFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         constrained = true;
         await(receiver, callbackModel, () -> receiverCallbacks.timeConstrainedEnabled,
            "receiver time-constrained enablement");
         publisher.enableTimeRegulation(publisherEpsilon);
         regulating = true;
         await(publisher, callbackModel, () -> publisherCallbacks.timeRegulationEnabled,
            "publisher time-regulation enablement");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "timed regional members began at different logical times");

         LogicalTime saveTime = offset(publisherInitial, publisherEpsilon, 6);
         LogicalTime queuedTime = offset(publisherInitial, publisherEpsilon, 8);
         LogicalTime releaseTime = offset(publisherInitial, publisherEpsilon, 10);
         JavaTckSupport.check(saveTime.equals(offset(receiverInitial, receiverEpsilon, 6)),
            "publisher and receiver derived different save frontiers");
         byte[] payload = defaultRegion
            ? new byte[] {0x54, 0x49, 0x4d, 0x45, 0x44}
            : new byte[] {0x54, 0x49, 0x4d, 0x45};
         byte[] tag = defaultRegion
            ? new byte[] {0x44, 0x45, 0x46, 0x2d, 0x54, 0x53, 0x4f}
            : new byte[] {0x52, 0x45, 0x47};
         ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload.clone());
         MessageRetractionReturn sent = defaultRegion
            ? publisher.sendInteraction(publisherInteraction, values, tag.clone(), queuedTime)
            : publisher.sendInteractionWithRegions(
               publisherInteraction, values, sourceRegions, tag.clone(), queuedTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped regional send returned an invalid message-retraction handle");
         JavaTckSupport.check(receiverCallbacks.interactions.isEmpty(),
            "time-constrained receiver got the interaction before the save frontier");

         publisher.requestFederationSave(label, saveTime);
         receiver.timeAdvanceRequest(offset(receiverInitial, receiverEpsilon, 6));
         publisher.timeAdvanceRequest(saveTime);
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherCallbacks.saveInitiations.size() == 1
               && receiverCallbacks.saveInitiations.size() == 1,
            "timed regional save initiation");
         verifySave(publisherCallbacks.saveInitiations.get(0), label, saveTime);
         verifySave(receiverCallbacks.saveInitiations.get(0), label, saveTime);
         JavaTckSupport.check(receiverCallbacks.interactions.isEmpty(),
            "save frontier delivered the still-queued regional interaction");

         publisher.federateSaveBegun();
         receiver.federateSaveBegun();
         publisher.federateSaveComplete();
         receiver.federateSaveComplete();
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherCallbacks.savedCount == 1 && receiverCallbacks.savedCount == 1,
            "timed regional federationSaved callbacks");
         JavaTckSupport.check(publisherCallbacks.saveFailures.isEmpty()
               && receiverCallbacks.saveFailures.isEmpty(),
            "timed regional federation save reported failure");

         // Remove the queued message in the live branch; restore must make this same handle live again.
         publisher.retract(sent.handle);
         publisherCallbacks.callbackOrder.clear();
         receiverCallbacks.clearDeliveryEvents();
         publisher.requestFederationRestore(label);
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherCallbacks.restoreSuccesses.size() == 1
               && publisherCallbacks.restoreBegunCount == 1 && receiverCallbacks.restoreBegunCount == 1
               && publisherCallbacks.restoreInitiations.size() == 1
               && receiverCallbacks.restoreInitiations.size() == 1,
            "timed regional restore initiation");
         JavaTckSupport.check(label.equals(publisherCallbacks.restoreSuccesses.get(0))
               && publisherCallbacks.restoreRequestFailures.isEmpty()
               && receiverCallbacks.restoreRequestFailures.isEmpty(),
            "timed regional restore request returned an unexpected result");
         RestoreInitiation publisherRestore = publisherCallbacks.restoreInitiations.get(0);
         RestoreInitiation receiverRestore = receiverCallbacks.restoreInitiations.get(0);
         verifyRestore(publisherRestore, label, "java-tck-timed-regional-publisher");
         verifyRestore(receiverRestore, label, "java-tck-timed-regional-receiver");

         publisher.federateRestoreComplete();
         receiver.federateRestoreComplete();
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherCallbacks.restoredCount == 1 && receiverCallbacks.restoredCount == 1,
            "timed regional federationRestored callbacks");
         JavaTckSupport.check(publisherCallbacks.restoreFailures.isEmpty()
               && receiverCallbacks.restoreFailures.isEmpty(),
            "timed regional federation restore reported failure");
         JavaTckSupport.check(publisher.getFederateHandle("java-tck-timed-regional-publisher")
               .equals(publisherRestore.postRestoreHandle)
               && receiver.getFederateHandle("java-tck-timed-regional-receiver")
                  .equals(receiverRestore.postRestoreHandle),
            "post-restore federate handles were not authoritative");

         receiverCallbacks.clearDeliveryEvents();
         receiver.flushQueueRequest(releaseTime);
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() == 1
               && receiverCallbacks.flushGrantTimes.size() == 1,
            "restored regional interaction and Flush Queue grant");
         TimedInteraction restored = receiverCallbacks.interactions.get(0);
         JavaTckSupport.check(receiverInteraction.equals(restored.interaction)
               && restored.parameters.size() == 1
               && restored.parameters.containsKey(receiverParameter)
               && Arrays.equals(payload, restored.parameters.get(receiverParameter))
               && Arrays.equals(tag, restored.tag)
               && publisherRestore.postRestoreHandle.equals(restored.producer)
               && queuedTime.equals(restored.time)
               && restored.sentOrder == OrderType.TIMESTAMP
               && restored.receivedOrder == OrderType.TIMESTAMP
               && sent.handle.equals(restored.retraction)
               && restored.regions != null
               && (defaultRegion ? restored.regions.isEmpty()
                  : restored.regions.size() == 1 && restored.regions.contains(sourceRegion)),
            "restored regional interaction changed its public delivery data");
         JavaTckSupport.check(!receiver.getTransportationTypeName(restored.transportation).isEmpty()
               && releaseTime.equals(receiverCallbacks.flushGrantTimes.get(0))
               && receiverCallbacks.callbackOrder.equals(Arrays.asList(
                  "receiveInteraction", "flushQueueGrant")),
            "restored interaction returned invalid transport, Flush Queue grant, or callback order");

         publisher.retract(sent.handle);
         await(receiver, callbackModel, () -> receiverCallbacks.retractions.size() == 1,
            "restored interaction requestRetraction callback");
         JavaTckSupport.check(sent.handle.equals(receiverCallbacks.retractions.get(0)),
            "restored requestRetraction callback returned a different handle");
         JavaTckSupport.expectFailure(() -> publisher.retract(sent.handle),
            "MessageCanNoLongerBeRetracted",
            "second retraction of the restored regional interaction was accepted");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         receiver.deleteRegion(receiverRegion);
         receiverRegion = null;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined && receiverRegions != null) try {
            receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         deleteRegion(publisher, sourceRegion, publisherJoined);
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long firstLower, long firstUpper, long secondLower, long secondUpper)
         throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, second, new RangeBounds(secondLower, secondUpper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle... regions)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      for (RegionHandle region : regions) result.add(region);
      return result;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void verifySave(SaveInitiation actual, String label, LogicalTime<?, ?> time) {
      JavaTckSupport.check(label.equals(actual.label) && time.equals(actual.time),
         "timed regional save initiation returned the wrong label or time");
   }

   private static void verifyRestore(RestoreInitiation actual, String label, String name) {
      JavaTckSupport.check(label.equals(actual.label) && name.equals(actual.federateName)
            && actual.postRestoreHandle != null && actual.postRestoreHandle.encodedLength() > 0,
         "timed regional restore initiation returned invalid identity metadata");
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

   private static void awaitBoth(RTIambassador first, RTIambassador second, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         pump(second, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region, boolean joined) {
      if (joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static final class SaveInitiation {
      private final String label;
      private final LogicalTime<?, ?> time;
      private SaveInitiation(String label, LogicalTime<?, ?> time) { this.label = label; this.time = time; }
   }

   private static final class RestoreInitiation {
      private final String label;
      private final String federateName;
      private final FederateHandle postRestoreHandle;
      private RestoreInitiation(String label, String name, FederateHandle handle) {
         this.label = label; federateName = name; postRestoreHandle = handle;
      }
   }

   private static final class TimedInteraction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final Set<RegionHandle> regions;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private TimedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         RegionHandleSet designators = (RegionHandleSet) arguments[5];
         regions = designators == null ? null : new HashSet<>(designators);
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled, timeConstrainedEnabled;
      private final List<SaveInitiation> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> saveFailures = new CopyOnWriteArrayList<>();
      private final List<TimedInteraction> interactions = new CopyOnWriteArrayList<>();
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreRequestFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> restoreFailures = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushGrantTimes = new CopyOnWriteArrayList<>();
      private volatile int savedCount, restoreBegunCount, restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled = true;
               else if ("timeConstrainedEnabled".equals(name)) timeConstrainedEnabled = true;
               else if ("initiateFederateSave".equals(name)) saveInitiations.add(new SaveInitiation(
                  (String) arguments[0], arguments.length > 1 ? (LogicalTime<?, ?>) arguments[1] : null));
               else if ("federationSaved".equals(name)) savedCount++;
               else if ("federationNotSaved".equals(name)) saveFailures.add((SaveFailureReason) arguments[0]);
               else if ("requestFederationRestoreSucceeded".equals(name))
                  restoreSuccesses.add((String) arguments[0]);
               else if ("requestFederationRestoreFailed".equals(name))
                  restoreRequestFailures.add((String) arguments[0]);
               else if ("federationRestoreBegun".equals(name)) restoreBegunCount++;
               else if ("initiateFederateRestore".equals(name)) restoreInitiations.add(
                  new RestoreInitiation((String) arguments[0], (String) arguments[1],
                     (FederateHandle) arguments[2]));
               else if ("federationRestored".equals(name)) restoredCount++;
               else if ("federationNotRestored".equals(name)) restoreFailures.add(arguments[0]);
               else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  interactions.add(new TimedInteraction(arguments));
                  callbackOrder.add(name);
               } else if ("flushQueueGrant".equals(name) && arguments != null
                     && arguments.length == 2) {
                  flushGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               } else if ("requestRetraction".equals(name) && arguments != null
                     && arguments.length == 1) {
                  retractions.add((MessageRetractionHandle) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timed regional save/restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private void clearDeliveryEvents() {
         interactions.clear();
         retractions.clear();
         callbackOrder.clear();
         flushGrantTimes.clear();
      }
   }
}
