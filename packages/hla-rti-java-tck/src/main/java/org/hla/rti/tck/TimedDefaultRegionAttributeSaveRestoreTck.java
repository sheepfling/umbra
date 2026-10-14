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
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.TransportationTypeHandle;
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

/** Restores a queued default-region timestamped attribute update and its retraction state. */
final class TimedDefaultRegionAttributeSaveRestoreTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private TimedDefaultRegionAttributeSaveRestoreTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-timed-default-attribute-save-restore-" + UUID.randomUUID();
      String label = "java-tck-timed-default-region-attribute-save-restore";
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean regulating = false, constrained = false, published = false, subscribed = false;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandleSet publisherAttributes = null, receiverAttributes = null;
      AttributeSetRegionSetPairList receiverAssociations = null;
      RegionHandle receiverRegion = null;
      ObjectInstanceHandle object = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-timed-default-attribute-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-timed-default-attribute-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute)
               && publisherFirst.equals(receiverFirst) && publisherSecond.equals(receiverSecond)
               && publisher.getAvailableDimensionsForObjectClass(publisherClass)
                  .contains(publisherFirst)
               && publisher.getAvailableDimensionsForObjectClass(publisherClass)
                  .contains(publisherSecond)
               && publisher.getDimensionUpperBound(publisherFirst) >= 3
               && publisher.getDimensionUpperBound(publisherSecond) >= 3,
            "adapter DDM object class, attribute, or dimension handles/metadata differed");

         publisherAttributes = attributeSet(publisher, publisherAttribute);
         receiverAttributes = attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(publisherClass, publisherAttributes,
            OrderType.TIMESTAMP);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 3, 2, 3);
         RegionHandleSet receiverRegions = regionSet(receiver, receiverRegion);
         receiverAssociations = association(receiver, receiverAttributes, receiverRegions);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "timed default-region attribute restore did not enable source-region designators");
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverAssociations);
         subscribed = true;

         object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null, "timed default-region registration returned no handle");
         ObjectInstanceHandle discoveredObject = object;
         await(receiver, callbackModel, () -> receiverCallbacks.discoveries.contains(discoveredObject),
            "registered object discovery");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "timed default-region attribute members selected different logical-time factories");
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
            "timed default-region attribute members began at different logical times");
         LogicalTime saveTime = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime queuedTime = offset(publisherInitial, publisherEpsilon, 7);
         LogicalTime releaseTime = offset(publisherInitial, publisherEpsilon, 10);
         JavaTckSupport.check(saveTime.equals(offset(receiverInitial, receiverEpsilon, 5)),
            "publisher and receiver derived different save frontiers");

         byte[] value = new byte[] {0x55, 0x4e, 0x54, 0x2d, 0x41, 0x54, 0x54};
         byte[] tag = new byte[] {0x55, 0x4e, 0x54, 0x2d, 0x54, 0x53, 0x4f};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttributes.iterator().next(), value.clone());
         MessageRetractionReturn sent = publisher.updateAttributeValues(
            object, values, tag.clone(), queuedTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped attribute update returned an invalid retraction handle");
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty(),
            "time-constrained receiver reflected the update before the save frontier");

         publisher.requestFederationSave(label, saveTime);
         receiver.timeAdvanceRequest(offset(receiverInitial, receiverEpsilon, 5));
         publisher.timeAdvanceRequest(saveTime);
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherCallbacks.saveInitiations.size() == 1
               && receiverCallbacks.saveInitiations.size() == 1,
            "timed default-region attribute save initiation");
         verifySave(publisherCallbacks.saveInitiations.get(0), label, saveTime);
         verifySave(receiverCallbacks.saveInitiations.get(0), label, saveTime);
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty(),
            "save frontier delivered the queued default-region reflection");

         publisher.federateSaveBegun();
         receiver.federateSaveBegun();
         publisher.federateSaveComplete();
         receiver.federateSaveComplete();
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherCallbacks.savedCount == 1 && receiverCallbacks.savedCount == 1,
            "timed default-region attribute federationSaved callbacks");
         JavaTckSupport.check(publisherCallbacks.saveFailures.isEmpty()
               && receiverCallbacks.saveFailures.isEmpty(),
            "timed default-region attribute save reported failure");

         publisher.retract(sent.handle);
         publisherCallbacks.callbackOrder.clear();
         receiverCallbacks.clearDeliveryEvents();
         publisher.requestFederationRestore(label);
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherCallbacks.restoreSuccesses.size() == 1
               && publisherCallbacks.restoreBegunCount == 1 && receiverCallbacks.restoreBegunCount == 1
               && publisherCallbacks.restoreInitiations.size() == 1
               && receiverCallbacks.restoreInitiations.size() == 1,
            "timed default-region attribute restore initiation");
         JavaTckSupport.check(label.equals(publisherCallbacks.restoreSuccesses.get(0))
               && publisherCallbacks.restoreRequestFailures.isEmpty()
               && receiverCallbacks.restoreRequestFailures.isEmpty(),
            "timed default-region attribute restore request returned an unexpected result");
         RestoreInitiation publisherRestore = publisherCallbacks.restoreInitiations.get(0);
         RestoreInitiation receiverRestore = receiverCallbacks.restoreInitiations.get(0);
         verifyRestore(publisherRestore, label, "java-tck-timed-default-attribute-publisher");
         verifyRestore(receiverRestore, label, "java-tck-timed-default-attribute-receiver");

         publisher.federateRestoreComplete();
         receiver.federateRestoreComplete();
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherCallbacks.restoredCount == 1 && receiverCallbacks.restoredCount == 1,
            "timed default-region attribute federationRestored callbacks");
         JavaTckSupport.check(publisherCallbacks.restoreFailures.isEmpty()
               && receiverCallbacks.restoreFailures.isEmpty(),
            "timed default-region attribute restore reported failure");
         JavaTckSupport.check(publisher.getFederateHandle("java-tck-timed-default-attribute-publisher")
               .equals(publisherRestore.postRestoreHandle)
               && receiver.getFederateHandle("java-tck-timed-default-attribute-receiver")
                  .equals(receiverRestore.postRestoreHandle),
            "post-restore attribute federate handles were not authoritative");

         receiverCallbacks.clearDeliveryEvents();
         receiver.flushQueueRequest(releaseTime);
         await(receiver, callbackModel,
            () -> receiverCallbacks.reflections.size() == 1
               && receiverCallbacks.flushGrantTimes.size() == 1,
            "restored default-region attribute reflection and Flush Queue grant");
         TimedReflection restored = receiverCallbacks.reflections.get(0);
         AttributeHandle expectedAttribute = receiverAttributes.iterator().next();
         JavaTckSupport.check(object.equals(restored.object)
               && restored.values.size() == 1
               && Arrays.equals(value, restored.values.get(expectedAttribute))
               && Arrays.equals(tag, restored.tag)
               && publisherRestore.postRestoreHandle.equals(restored.producer)
               && queuedTime.equals(restored.time)
               && restored.sentOrder == OrderType.TIMESTAMP
               && restored.receivedOrder == OrderType.TIMESTAMP
               && sent.handle.equals(restored.retraction)
               && restored.regions != null && restored.regions.isEmpty(),
            "restored default-region attribute reflection changed its public delivery data");
         LogicalTime<?, ?> grantTime = receiverCallbacks.flushGrantTimes.get(0);
         LogicalTime<?, ?> optimisticTime = receiverCallbacks.flushOptimisticTimes.get(0);
         JavaTckSupport.check(!receiver.getTransportationTypeName(restored.transportation).isEmpty()
               && releaseTime.equals(grantTime) && queuedTime.equals(optimisticTime)
               && !grantTime.toString().isEmpty() && queuedTime.toString().equals(
                  receiverCallbacks.flushOptimisticTimeTexts.get(0))
               && receiverCallbacks.callbackOrder.equals(Arrays.asList(
                  "reflectAttributeValues", "flushQueueGrant")),
            "restored reflection returned wrong transport, Flush Queue frontier, or callback order");

         publisher.retract(sent.handle);
         await(receiver, callbackModel, () -> receiverCallbacks.retractions.size() == 1,
            "restored default-region attribute requestRetraction callback");
         JavaTckSupport.check(sent.handle.equals(receiverCallbacks.retractions.get(0)),
            "restored attribute retraction callback returned a different handle");
         JavaTckSupport.expectFailure(() -> publisher.retract(sent.handle),
            "MessageCanNoLongerBeRetracted",
            "second retraction of the restored default-region attribute was accepted");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverAssociations);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         receiver.deleteRegion(receiverRegion);
         receiverRegion = null;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined && receiverAssociations != null) try {
            receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverAssociations);
         } catch (Exception ignored) { }
         if (published && publisherJoined && publisherClass != null) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegion, receiverJoined);
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

   private static AttributeHandleSet attributeSet(RTIambassador ambassador,
         AttributeHandle attribute) throws Exception {
      AttributeHandleSet result = ambassador.getAttributeHandleSetFactory().create();
      result.add(attribute);
      return result;
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

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandleSet regions) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regions));
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
         "timed attribute save initiation returned the wrong label or time");
   }

   private static void verifyRestore(RestoreInitiation actual, String label, String name) {
      JavaTckSupport.check(label.equals(actual.label) && name.equals(actual.federateName)
            && actual.postRestoreHandle != null && actual.postRestoreHandle.encodedLength() > 0,
         "timed attribute restore initiation returned invalid identity metadata");
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

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK timed attribute publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
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

   private static final class TimedReflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final Set<RegionHandle> regions;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private TimedReflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
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
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<SaveInitiation> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> saveFailures = new CopyOnWriteArrayList<>();
      private final List<TimedReflection> reflections = new CopyOnWriteArrayList<>();
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreRequestFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> restoreFailures = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushGrantTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushOptimisticTimes = new CopyOnWriteArrayList<>();
      private final List<String> flushOptimisticTimeTexts = new CopyOnWriteArrayList<>();
      private volatile int savedCount, restoreBegunCount, restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null)
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               else if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled = true;
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
               else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  reflections.add(new TimedReflection(arguments));
                  callbackOrder.add(name);
               } else if ("flushQueueGrant".equals(name) && arguments != null
                     && arguments.length == 2) {
                  LogicalTime<?, ?> grant = (LogicalTime<?, ?>) arguments[0];
                  LogicalTime<?, ?> optimistic = (LogicalTime<?, ?>) arguments[1];
                  flushGrantTimes.add(grant);
                  flushOptimisticTimes.add(optimistic);
                  flushOptimisticTimeTexts.add(optimistic.toString());
                  callbackOrder.add(name);
               } else if ("requestRetraction".equals(name) && arguments != null
                     && arguments.length == 1) retractions.add((MessageRetractionHandle) arguments[0]);
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timed default-attribute callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private void clearDeliveryEvents() {
         reflections.clear();
         retractions.clear();
         callbackOrder.clear();
         flushGrantTimes.clear();
         flushOptimisticTimes.clear();
         flushOptimisticTimeTexts.clear();
      }
   }
}
