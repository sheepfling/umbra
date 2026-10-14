package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Continues a saved timestamped regional attribute and ownership candidate. */
final class TimedRegionalAttributeOwnershipSaveRestoreTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;
   private static final byte[] VALUE = new byte[] {0x44, 0x45, 0x4c, 0x45, 0x54, 0x45};
   private static final byte[] UPDATE_TAG = new byte[] {0x52, 0x45, 0x53, 0x54, 0x4f, 0x52, 0x45};
   private static final byte[] FIRST_ACQUISITION_TAG = new byte[] {0x41, 0x43, 0x51, 0x2d, 0x31};
   private static final byte[] CLOCK_ACQUISITION_TAG = new byte[] {0x41, 0x43, 0x51, 0x2d, 0x32};
   private static final byte[] NEGOTIATED_TAG = new byte[] {0x4e, 0x45, 0x47, 0x2d, 0x54};
   private static final byte[] CONFIRMATION_TAG = new byte[] {0x43, 0x4f, 0x4e, 0x2d, 0x54};

   private TimedRegionalAttributeOwnershipSaveRestoreTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      runInternal(factory, fom, timeImplementation, objectClassName, attributeName,
         firstDimensionName, secondDimensionName, callbackModel, false, false);
   }

   static void runCancelAfterConfirmation(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      runInternal(factory, fom, timeImplementation, objectClassName, attributeName,
         firstDimensionName, secondDimensionName, callbackModel, true, false);
   }

   static void runCancelBeforeConfirmation(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      runInternal(factory, fom, timeImplementation, objectClassName, attributeName,
         firstDimensionName, secondDimensionName, callbackModel, false, true);
   }

   private static void runInternal(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel, boolean cancelAfterConfirmation,
         boolean cancelBeforeConfirmation)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder firstEvents = new Recorder();
      Recorder secondEvents = new Recorder();
      Recorder clockEvents = new Recorder();
      String federation = "java-tck-timed-regional-owner-restore-" + UUID.randomUUID();
      String label = "java-tck-timed-regional-owner-candidate-continuation";
      String publisherName = "java-tck-timed-owner-publisher";
      String firstName = "java-tck-timed-owner-first";
      String secondName = "java-tck-timed-owner-second";
      String clockName = "java-tck-timed-owner-clock";
      boolean publisherConnected = false, firstConnected = false;
      boolean secondConnected = false, clockConnected = false, created = false;
      boolean publisherJoined = false, firstJoined = false;
      boolean secondJoined = false, clockJoined = false;
      boolean regulating = false, clockRegulating = false, firstConstrained = false;
      boolean secondConstrained = false;
      RegionHandle sourceRegion = null, firstRegion = null, secondRegion = null;
      ObjectClassHandle objectClass = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet firstAttributes = null;
      AttributeHandleSet secondAttributes = null;
      AttributeHandleSet clockAttributes = null;
      AttributeSetRegionSetPairList publisherAssociations = null;
      AttributeSetRegionSetPairList firstAssociations = null;
      AttributeSetRegionSetPairList secondAssociations = null;
      ObjectInstanceHandle object = null;
      String objectName = null;
      ObjectClassHandle firstClassHandle = null, secondClassHandle = null, clockClassHandle = null;
      try {
         publisher.connect(publisherEvents.proxy(), callbackModel);
         publisherConnected = true;
         first.connect(firstEvents.proxy(), callbackModel);
         firstConnected = true;
         second.connect(secondEvents.proxy(), callbackModel);
         secondConnected = true;
         clock.connect(clockEvents.proxy(), callbackModel);
         clockConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution(publisherName, federation);
         publisherJoined = true;
         first.joinFederationExecution(firstName, federation);
         firstJoined = true;
         second.joinFederationExecution(secondName, federation);
         secondJoined = true;
         clock.joinFederationExecution(clockName, federation);
         clockJoined = true;

         objectClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = first.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = second.getObjectClassHandle(objectClassName);
         ObjectClassHandle clockClass = clock.getObjectClassHandle(objectClassName);
         firstClassHandle = firstClass;
         secondClassHandle = secondClass;
         clockClassHandle = clockClass;
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(objectClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         AttributeHandle clockAttribute = clock.getAttributeHandle(clockClass, attributeName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle firstFirst = first.getDimensionHandle(firstDimensionName);
         DimensionHandle firstSecond = first.getDimensionHandle(secondDimensionName);
         DimensionHandle secondFirst = second.getDimensionHandle(firstDimensionName);
         DimensionHandle secondSecond = second.getDimensionHandle(secondDimensionName);
         DimensionHandle clockFirst = clock.getDimensionHandle(firstDimensionName);
         DimensionHandle clockSecond = clock.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(objectClass.equals(firstClass) && objectClass.equals(secondClass)
               && objectClass.equals(clockClass) && publisherAttribute.equals(firstAttribute)
               && publisherAttribute.equals(secondAttribute) && publisherAttribute.equals(clockAttribute)
               && publisherFirst.equals(firstFirst) && publisherFirst.equals(secondFirst)
               && publisherFirst.equals(clockFirst) && publisherSecond.equals(firstSecond)
               && publisherSecond.equals(secondSecond) && publisherSecond.equals(clockSecond),
            "adapter FOM handles differed across timed regional ownership participants");
         JavaTckSupport.check(publisher.getAvailableDimensionsForObjectClass(objectClass)
               .contains(publisherFirst)
               && publisher.getAvailableDimensionsForObjectClass(objectClass).contains(publisherSecond)
               && publisher.getDimensionUpperBound(publisherFirst) >= 4
               && publisher.getDimensionUpperBound(publisherSecond) >= 3,
            "adapter object class did not expose the required DDM dimensions and bounds");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         secondAttributes = JavaTckSupport.attributeSet(second, secondAttribute);
         clockAttributes = JavaTckSupport.attributeSet(clock, clockAttribute);
         publisher.publishObjectClassAttributes(objectClass, publisherAttributes);
         publisher.changeDefaultAttributeOrderType(objectClass, publisherAttributes, OrderType.TIMESTAMP);
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 0, 2, 0, 2);
         firstRegion = createRegion(first, firstFirst, firstSecond, 1, 3, 0, 2);
         secondRegion = createRegion(second, secondFirst, secondSecond, 0, 1, 0, 2);
         publisherAssociations = association(publisher, publisherAttributes,
            regionSet(publisher, sourceRegion));
         firstAssociations = association(first, firstAttributes, regionSet(first, firstRegion));
         secondAssociations = association(second, secondAttributes, regionSet(second, secondRegion));
         first.setConveyRegionDesignatorSetsSwitch(true);
         second.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(first.getConveyRegionDesignatorSetsSwitch()
               && second.getConveyRegionDesignatorSetsSwitch(),
            "regional receivers did not enable source-region designators");
         first.subscribeObjectClassAttributesWithRegions(firstClass, firstAssociations);
         second.subscribeObjectClassAttributesWithRegions(secondClass, secondAssociations);
         clock.subscribeObjectClassAttributes(clockClass, clockAttributes);
         object = publisher.registerObjectInstanceWithRegions(objectClass, publisherAssociations);
         JavaTckSupport.check(object != null, "regional object registration returned no handle");
         objectName = publisher.getObjectInstanceName(object);
         ObjectInstanceHandle registeredObject = object;
         awaitAll(publisher, first, second, clock, callbackModel,
            () -> firstEvents.discoveries.contains(registeredObject)
               && secondEvents.discoveries.contains(registeredObject)
               && clockEvents.discoveries.contains(registeredObject), "object discovery");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> firstFactory = first.getTimeFactory();
         LogicalTimeFactory<?, ?> secondFactory = second.getTimeFactory();
         LogicalTimeFactory<?, ?> clockFactory = clock.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(firstFactory.getName())
               && publisherFactory.getName().equals(secondFactory.getName())
               && publisherFactory.getName().equals(clockFactory.getName()),
            "members selected different logical-time factories");
         LogicalTimeInterval publisherEpsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval firstEpsilon = (LogicalTimeInterval) firstFactory.makeEpsilon();
         LogicalTimeInterval secondEpsilon = (LogicalTimeInterval) secondFactory.makeEpsilon();
         LogicalTimeInterval clockEpsilon = (LogicalTimeInterval) clockFactory.makeEpsilon();
         first.enableTimeConstrained();
         firstConstrained = true;
         await(first, callbackModel, () -> firstEvents.timeConstrainedEnabled, "first constrained callback");
         second.enableTimeConstrained();
         secondConstrained = true;
         await(second, callbackModel, () -> secondEvents.timeConstrainedEnabled, "second constrained callback");
         publisher.enableTimeRegulation(publisherEpsilon);
         regulating = true;
         await(publisher, callbackModel, () -> publisherEvents.timeRegulationEnabled, "publisher regulation callback");
         clock.enableTimeRegulation(clockEpsilon);
         clockRegulating = true;
         await(clock, callbackModel, () -> clockEvents.timeRegulationEnabled, "clock regulation callback");
         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime firstInitial = first.queryLogicalTime();
         LogicalTime secondInitial = second.queryLogicalTime();
         LogicalTime clockInitial = clock.queryLogicalTime();
         JavaTckSupport.check(initial.equals(firstInitial) && initial.equals(secondInitial)
               && initial.equals(clockInitial), "members began at different logical times");
         LogicalTime saveTime = offset(initial, publisherEpsilon, 6);
         LogicalTime messageTime = offset(initial, publisherEpsilon, 8);
         LogicalTime publisherGrant = offset(initial, publisherEpsilon, 9);
         LogicalTime flushTime = offset(initial, publisherEpsilon, 10);
         JavaTckSupport.check(saveTime.equals(offset(firstInitial, firstEpsilon, 6))
               && saveTime.equals(offset(secondInitial, secondEpsilon, 6))
               && saveTime.equals(offset(clockInitial, clockEpsilon, 6)),
            "participants derived different save frontiers");

         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, VALUE.clone());
         MessageRetractionReturn sent = publisher.updateAttributeValues(
            object, values, UPDATE_TAG.clone(), messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && firstEvents.reflections.isEmpty() && secondEvents.reflections.isEmpty(),
            "timestamped update returned an invalid handle or escaped the constrained frontier");
         publisher.requestFederationSave(label, saveTime);
         first.timeAdvanceRequest(offset(firstInitial, firstEpsilon, 6));
         second.timeAdvanceRequest(offset(secondInitial, secondEpsilon, 6));
         publisher.timeAdvanceRequest(saveTime);
         clock.timeAdvanceRequest(offset(clockInitial, clockEpsilon, 6));
         awaitAll(publisher, first, second, clock, callbackModel,
            () -> publisherEvents.saveInitiations.size() == 1
               && firstEvents.saveInitiations.size() == 1 && secondEvents.saveInitiations.size() == 1
               && clockEvents.saveInitiations.size() == 1, "timed save initiation");
         verifySave(publisherEvents, label, saveTime);
         verifySave(firstEvents, label, saveTime);
         verifySave(secondEvents, label, saveTime);
         verifySave(clockEvents, label, saveTime);
         publisher.federateSaveBegun();
         first.federateSaveBegun();
         second.federateSaveBegun();
         clock.federateSaveBegun();
         publisher.federateSaveComplete();
         first.federateSaveComplete();
         second.federateSaveComplete();
         clock.federateSaveComplete();
         awaitAll(publisher, first, second, clock, callbackModel,
            () -> publisherEvents.savedCount == 1 && firstEvents.savedCount == 1
               && secondEvents.savedCount == 1 && clockEvents.savedCount == 1,
            "federation saved callbacks");
         JavaTckSupport.check(publisherEvents.saveFailures.isEmpty() && firstEvents.saveFailures.isEmpty()
               && secondEvents.saveFailures.isEmpty() && clockEvents.saveFailures.isEmpty()
               && firstEvents.reflections.isEmpty() && secondEvents.reflections.isEmpty(),
            "save frontier delivered the queued attribute update");

         publisher.setRangeBounds(sourceRegion, publisherFirst, new RangeBounds(0, 1));
         publisher.commitRegionModifications(regionSet(publisher, sourceRegion));
         publisher.requestFederationRestore(label);
         awaitAll(publisher, first, second, clock, callbackModel,
            () -> publisherEvents.restoreSuccesses.size() == 1
               && publisherEvents.restoreInitiations.size() == 1
               && firstEvents.restoreInitiations.size() == 1
               && secondEvents.restoreInitiations.size() == 1
               && clockEvents.restoreInitiations.size() == 1
               && publisherEvents.restoreBegunCount == 1 && firstEvents.restoreBegunCount == 1
               && secondEvents.restoreBegunCount == 1 && clockEvents.restoreBegunCount == 1,
            "timed restore initiation");
         JavaTckSupport.check(label.equals(last(publisherEvents.restoreSuccesses))
               && publisherEvents.restoreFailures.isEmpty() && firstEvents.restoreFailures.isEmpty()
               && secondEvents.restoreFailures.isEmpty() && clockEvents.restoreFailures.isEmpty(),
            "restore request failed or returned the wrong label");
         publisher.federateRestoreComplete();
         first.federateRestoreComplete();
         second.federateRestoreComplete();
         clock.federateRestoreComplete();
         awaitAll(publisher, first, second, clock, callbackModel,
            () -> publisherEvents.restoredCount == 1 && firstEvents.restoredCount == 1
               && secondEvents.restoredCount == 1 && clockEvents.restoredCount == 1,
            "federation restored callbacks");
         JavaTckSupport.check(publisherEvents.restoreFailures.isEmpty() && firstEvents.restoreFailures.isEmpty()
               && secondEvents.restoreFailures.isEmpty() && clockEvents.restoreFailures.isEmpty(),
            "federation restore reported failure");
         RangeBounds restoredBounds = publisher.getRangeBounds(sourceRegion, publisherFirst);
         JavaTckSupport.check(restoredBounds.lower == 0 && restoredBounds.upper == 2,
            "restore did not recover the saved source region");
         RestoreInitiation publisherRestore = last(publisherEvents.restoreInitiations);
         RestoreInitiation firstRestore = last(firstEvents.restoreInitiations);
         RestoreInitiation secondRestore = last(secondEvents.restoreInitiations);
         RestoreInitiation clockRestore = last(clockEvents.restoreInitiations);
         JavaTckSupport.check(publisherRestore.matches(label, publisherName)
               && firstRestore.matches(label, firstName) && secondRestore.matches(label, secondName)
               && clockRestore.matches(label, clockName)
               && publisherRestore.postRestoreHandle.equals(publisher.getFederateHandle(publisherName))
               && firstRestore.postRestoreHandle.equals(first.getFederateHandle(firstName))
               && secondRestore.postRestoreHandle.equals(second.getFederateHandle(secondName))
               && clockRestore.postRestoreHandle.equals(clock.getFederateHandle(clockName)),
            "restore did not preserve all federate identities");
         JavaTckSupport.check(object.equals(second.getObjectInstanceHandle(objectName)),
            "restore changed the discovered object's name-to-handle mapping");

         publisher.setRangeBounds(sourceRegion, publisherFirst, new RangeBounds(3, 4));
         publisher.commitRegionModifications(regionSet(publisher, sourceRegion));
         first.publishObjectClassAttributes(firstClass, firstAttributes);
         clock.publishObjectClassAttributes(clockClass, clockAttributes);
         boolean immediateTransfer = false;
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            first.attributeOwnershipAcquisitionIfAvailable(
               object, firstAttributes, FIRST_ACQUISITION_TAG.clone());
            JavaTckSupport.check(firstEvents.acquisitions.isEmpty()
                  && !first.isAttributeOwnedByFederate(object, firstAttribute),
               "unavailable immediate candidate unexpectedly acquired ownership");
            first.resignFederationExecution(ResignAction.NO_ACTION);
            firstJoined = false;
         } else {
            first.attributeOwnershipAcquisition(object, firstAttributes, FIRST_ACQUISITION_TAG.clone());
            clock.attributeOwnershipAcquisition(object, clockAttributes, CLOCK_ACQUISITION_TAG.clone());
            JavaTckSupport.check(firstEvents.acquisitions.isEmpty() && clockEvents.acquisitions.isEmpty(),
               "regular acquisition completed before the owner divested the attribute");
            publisherEvents.clearOwnership();
            publisher.negotiatedAttributeOwnershipDivestiture(
               object, publisherAttributes, NEGOTIATED_TAG.clone());
            JavaTckSupport.check(publisher.isAttributeOwnedByFederate(object, publisherAttribute)
                  && !clock.isAttributeOwnedByFederate(object, clockAttribute),
               "negotiated divestiture transferred before confirmation");
            first.resignFederationExecution(ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
            firstJoined = false;
            for (int pass = 0; pass < 8; ++pass) JavaTckSupport.drain(publisher);
            if (!publisherEvents.releaseRequests.isEmpty()) {
               OwnershipEvent releaseRequest = last(publisherEvents.releaseRequests);
               JavaTckSupport.check(object.equals(releaseRequest.object)
                     && releaseRequest.attributes.equals(new HashSet<>(publisherAttributes))
                     && Arrays.equals(CLOCK_ACQUISITION_TAG, releaseRequest.tag),
                  "resignation delivered a release request for the canceled candidate");
            }
            JavaTckSupport.check(publisherEvents.divestitureRequests.isEmpty()
                  && clockEvents.acquisitions.isEmpty(),
               "resignation delivered a stale confirmation or acquisition callback");
            firstEvents.clearOwnership();
            publisherEvents.clearOwnership();
            publisher.negotiatedAttributeOwnershipDivestiture(
               object, publisherAttributes, NEGOTIATED_TAG.clone());
            if (cancelBeforeConfirmation) {
               JavaTckSupport.check(publisherEvents.divestitureRequests.isEmpty(),
                  "pre-delivery cancellation crossed the confirmation callback boundary");
               publisher.cancelNegotiatedAttributeOwnershipDivestiture(object, publisherAttributes);
               JavaTckSupport.check(publisher.isAttributeOwnedByFederate(object, publisherAttribute)
                     && !clock.isAttributeOwnedByFederate(object, clockAttribute)
                     && publisherEvents.divestitureRequests.isEmpty()
                     && clockEvents.acquisitions.isEmpty(),
                  "pre-delivery cancellation changed ownership or delivered stale callbacks");
               clock.cancelAttributeOwnershipAcquisition(object, clockAttributes);
               await(clock, callbackModel, () -> !clockEvents.cancellations.isEmpty(),
                  "pre-delivery acquisition cancellation callback");
               OwnershipEvent cancellation = last(clockEvents.cancellations);
               JavaTckSupport.check(object.equals(cancellation.object)
                     && cancellation.attributes.equals(new HashSet<>(clockAttributes)),
                  "pre-delivery cancellation returned the wrong object or attributes");
            } else {
               await(publisher, callbackModel, () -> !publisherEvents.divestitureRequests.isEmpty(),
                  "retained-candidate divestiture request");
               OwnershipEvent request = last(publisherEvents.divestitureRequests);
               JavaTckSupport.check(object.equals(request.object)
                     && request.attributes.equals(new HashSet<>(publisherAttributes))
                     && Arrays.equals(CLOCK_ACQUISITION_TAG, request.tag),
                  "divestiture request did not retain the surviving candidate");
            }
         }

         secondEvents.reflections.clear();
         secondEvents.callbackOrder.clear();
         second.flushQueueRequest(flushTime);
         publisher.timeAdvanceRequest(publisherGrant);
         clock.timeAdvanceRequest(publisherGrant);
         awaitThree(publisher, second, clock, callbackModel,
            () -> publisherEvents.timeAdvanceGrants > 0 && clockEvents.timeAdvanceGrants > 0
               && secondEvents.reflections.size() == 1 && secondEvents.flushGrantCount == 1,
            "restored update and Flush Queue grant");
         Reflection reflection = last(secondEvents.reflections);
         JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
               && Arrays.equals(VALUE, reflection.values.get(secondAttribute))
               && Arrays.equals(UPDATE_TAG, reflection.tag)
               && reflection.time.equals(messageTime)
               && reflection.sentOrder == OrderType.TIMESTAMP
               && reflection.receivedOrder == OrderType.TIMESTAMP
               && reflection.retraction.equals(sent.handle)
               && reflection.producer.equals(publisherRestore.postRestoreHandle)
               && !second.getTransportationTypeName(reflection.transportation).isEmpty()
               && reflection.regions != null && reflection.regions.size() == 1
               && reflection.regions.contains(sourceRegion),
            "restored regional reflection changed its value, time, retraction, order, or source region");
         JavaTckSupport.check(secondEvents.callbackOrder.equals(
               Arrays.asList("reflectAttributeValues", "flushQueueGrant")),
            "restored reflection was not delivered before the Flush Queue grant");
         if (cancelBeforeConfirmation && !immediateTransfer) {
            JavaTckSupport.check(publisherEvents.divestitureRequests.isEmpty()
                  && publisher.isAttributeOwnedByFederate(object, publisherAttribute)
                  && !clock.isAttributeOwnedByFederate(object, clockAttribute),
               "pre-delivery cancellation later delivered a stale confirmation or changed ownership");
         }

         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            clock.attributeOwnershipAcquisition(object, clockAttributes, CLOCK_ACQUISITION_TAG.clone());
            await(publisher, callbackModel, () -> !publisherEvents.releaseRequests.isEmpty(),
               "immediate owner release request");
            OwnershipEvent release = last(publisherEvents.releaseRequests);
            JavaTckSupport.check(object.equals(release.object)
                  && release.attributes.equals(new HashSet<>(publisherAttributes))
                  && Arrays.equals(CLOCK_ACQUISITION_TAG, release.tag),
               "immediate release request did not identify the surviving candidate");
            publisher.attributeOwnershipDivestitureIfWanted(
               object, publisherAttributes, NEGOTIATED_TAG.clone());
            await(clock, callbackModel, () -> !clockEvents.acquisitions.isEmpty(),
               "immediate ownership notification");
            OwnershipEvent transfer = last(clockEvents.acquisitions);
            JavaTckSupport.check(object.equals(transfer.object)
                  && transfer.attributes.equals(new HashSet<>(clockAttributes))
                  && Arrays.equals(NEGOTIATED_TAG, transfer.tag)
                  && !publisher.isAttributeOwnedByFederate(object, publisherAttribute)
                  && clock.isAttributeOwnedByFederate(object, clockAttribute),
               "immediate ownership handoff did not transfer ownership");
            immediateTransfer = true;
         }
         if (cancelAfterConfirmation && !immediateTransfer) {
            publisher.cancelNegotiatedAttributeOwnershipDivestiture(object, publisherAttributes);
            JavaTckSupport.check(publisher.isAttributeOwnedByFederate(object, publisherAttribute)
                  && !clock.isAttributeOwnedByFederate(object, clockAttribute),
               "cancelling the confirmed negotiated transfer changed ownership");
            ObjectInstanceHandle cancelObject = object;
            AttributeHandleSet cancelAttributes = publisherAttributes;
            JavaTckSupport.expectFailure(
               () -> publisher.confirmDivestiture(
                  cancelObject, cancelAttributes, CONFIRMATION_TAG.clone()),
               "AttributeDivestitureWasNotRequested",
               "confirming a negotiated transfer after its cancellation");
            JavaTckSupport.check(publisherEvents.divestitureRequests.size() == 1
                  && clockEvents.acquisitions.isEmpty(),
               "cancellation delivered stale ownership callbacks");
            clock.cancelAttributeOwnershipAcquisition(object, clockAttributes);
            await(clock, callbackModel, () -> !clockEvents.cancellations.isEmpty(),
               "acquisition cancellation callback");
            OwnershipEvent cancellation = last(clockEvents.cancellations);
            JavaTckSupport.check(object.equals(cancellation.object)
                  && cancellation.attributes.equals(new HashSet<>(clockAttributes)),
               "acquisition cancellation returned the wrong object or attributes");
         } else if (!immediateTransfer && !cancelBeforeConfirmation) {
            publisher.confirmDivestiture(object, publisherAttributes, CONFIRMATION_TAG.clone());
            await(clock, callbackModel, () -> !clockEvents.acquisitions.isEmpty(),
               "post-restore ownership notification");
            OwnershipEvent acquisition = last(clockEvents.acquisitions);
            JavaTckSupport.check(object.equals(acquisition.object)
                  && acquisition.attributes.equals(new HashSet<>(clockAttributes))
                  && Arrays.equals(CONFIRMATION_TAG, acquisition.tag)
                  && !publisher.isAttributeOwnedByFederate(object, publisherAttribute)
                  && clock.isAttributeOwnedByFederate(object, clockAttribute),
               "confirmation did not transfer the saved ownership candidate");
         } else if (!immediateTransfer) {
            JavaTckSupport.check(publisher.isAttributeOwnedByFederate(object, publisherAttribute)
                  && !clock.isAttributeOwnedByFederate(object, clockAttribute)
                  && clockEvents.acquisitions.isEmpty(),
               "pre-delivery cancellation did not preserve ownership through queued delivery");
         }
      } finally {
         if (secondConstrained && secondJoined) try { second.disableTimeConstrained(); } catch (Exception ignored) { }
         if (firstConstrained && firstJoined) try { first.disableTimeConstrained(); } catch (Exception ignored) { }
         if (regulating && publisherJoined) try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         if (clockRegulating && clockJoined) try { clock.disableTimeRegulation(); } catch (Exception ignored) { }
         if (secondJoined && secondClassHandle != null && secondAssociations != null)
            try { second.unsubscribeObjectClassAttributesWithRegions(secondClassHandle, secondAssociations); }
            catch (Exception ignored) { }
         if (clockJoined && clockClassHandle != null && clockAttributes != null)
            try { clock.unsubscribeObjectClassAttributes(clockClassHandle, clockAttributes); }
            catch (Exception ignored) { }
         if (firstJoined && firstClassHandle != null && firstAttributes != null)
            try { first.unpublishObjectClassAttributes(firstClassHandle, firstAttributes); }
            catch (Exception ignored) { }
         if (publisherJoined && objectClass != null && publisherAttributes != null)
            try { publisher.unpublishObjectClassAttributes(objectClass, publisherAttributes); }
            catch (Exception ignored) { }
         if (clockJoined && clockClassHandle != null && clockAttributes != null)
            try { clock.unpublishObjectClassAttributes(clockClassHandle, clockAttributes); }
            catch (Exception ignored) { }
         deleteRegion(second, secondRegion, secondJoined);
         deleteRegion(first, firstRegion, firstJoined);
         if (secondJoined) resign(second, ResignAction.NO_ACTION);
         if (firstJoined) resign(first, ResignAction.NO_ACTION);
         if (clockJoined) resign(clock, ResignAction.NO_ACTION);
         if (publisherJoined) resign(publisher, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (created) try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         if (clockConnected) disconnect(clock);
         if (secondConnected) disconnect(second);
         if (firstConnected) disconnect(first);
         if (publisherConnected) disconnect(publisher);
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

   private static void verifySave(Recorder recorder, String label, LogicalTime time) {
      JavaTckSupport.check(recorder.saveInitiations.size() == 1
            && label.equals(recorder.saveInitiations.get(0)[0])
            && time.equals(recorder.saveInitiations.get(0)[1]),
         "timed save callback returned the wrong label or time");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      awaitMany(new RTIambassador[] {ambassador}, model, condition, description);
   }

   private static void awaitAll(RTIambassador a, RTIambassador b, RTIambassador c,
         RTIambassador d, CallbackModel model, BooleanSupplier condition, String description)
         throws Exception {
      awaitMany(new RTIambassador[] {a, b, c, d}, model, condition, description);
   }

   private static void awaitThree(RTIambassador a, RTIambassador b, RTIambassador c,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      awaitMany(new RTIambassador[] {a, b, c}, model, condition, description);
   }

   private static void awaitMany(RTIambassador[] ambassadors, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         for (RTIambassador ambassador : ambassadors) {
            if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         }
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region, boolean joined) {
      if (joined && region != null) try { ambassador.deleteRegion(region); } catch (Exception ignored) { }
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static <T> T last(List<T> values) { return values.get(values.size() - 1); }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final byte[] tag;
      private OwnershipEvent(Object[] args) {
         object = (ObjectInstanceHandle) args[0];
         attributes = new HashSet<>((AttributeHandleSet) args[1]);
         tag = args.length > 2 && args[2] instanceof byte[] ? ((byte[]) args[2]).clone() : new byte[0];
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final hla.rti1516_2025.TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final LogicalTime time;
      private final OrderType sentOrder, receivedOrder;
      private final Object retraction;
      private final HashSet<RegionHandle> regions;
      private Reflection(Object[] args) {
         object = (ObjectInstanceHandle) args[0];
         values = ((AttributeHandleValueMap) args[1]).clone();
         tag = ((byte[]) args[2]).clone();
         transportation = (hla.rti1516_2025.TransportationTypeHandle) args[3];
         producer = (FederateHandle) args[4];
         time = (LogicalTime) args[6];
         sentOrder = (OrderType) args[7];
         receivedOrder = (OrderType) args[8];
         retraction = args[9];
         RegionHandleSet set = (RegionHandleSet) args[5];
         regions = set == null ? null : new HashSet<>(set);
      }
   }

   private static final class RestoreInitiation {
      private final String label, federateName;
      private final FederateHandle postRestoreHandle;
      private RestoreInitiation(Object[] args) {
         label = (String) args[0];
         federateName = (String) args[1];
         postRestoreHandle = (FederateHandle) args[2];
      }
      private boolean matches(String expectedLabel, String expectedName) {
         return expectedLabel.equals(label) && expectedName.equals(federateName)
            && postRestoreHandle != null;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Object[]> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> saveFailures = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      private final List<Object> restoreFailures = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> cancellations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> divestitureRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile boolean timeRegulationEnabled, timeConstrainedEnabled;
      private volatile int savedCount, restoreBegunCount, restoredCount;
      private volatile int timeAdvanceGrants, flushGrantCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) discoveries.add((ObjectInstanceHandle) args[0]);
               else if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled = true;
               else if ("timeConstrainedEnabled".equals(name)) timeConstrainedEnabled = true;
               else if ("initiateFederateSave".equals(name)) saveInitiations.add(args.clone());
               else if ("federationSaved".equals(name)) savedCount++;
               else if ("federationNotSaved".equals(name)) saveFailures.add(args[0]);
               else if ("requestFederationRestoreSucceeded".equals(name)) restoreSuccesses.add((String) args[0]);
               else if ("requestFederationRestoreFailed".equals(name)
                     || "federationNotRestored".equals(name)) restoreFailures.add(args[0]);
               else if ("federationRestoreBegun".equals(name)) restoreBegunCount++;
               else if ("initiateFederateRestore".equals(name))
                  restoreInitiations.add(new RestoreInitiation(args));
               else if ("federationRestored".equals(name)) restoredCount++;
               else if ("reflectAttributeValues".equals(name) && args.length == 10) {
                  reflections.add(new Reflection(args));
                  callbackOrder.add(name);
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     || "requestDivestitureConfirmation".equals(name)) {
                  OwnershipEvent event = new OwnershipEvent(args);
                  if ("requestAttributeOwnershipRelease".equals(name)) releaseRequests.add(event);
                  else divestitureRequests.add(event);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)) {
                  acquisitions.add(new OwnershipEvent(args));
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)) {
                  cancellations.add(new OwnershipEvent(args));
               } else if ("timeAdvanceGrant".equals(name)) timeAdvanceGrants++;
               else if ("flushQueueGrant".equals(name)) {
                  flushGrantCount++;
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional ownership callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }

      private void clearOwnership() {
         acquisitions.clear();
         divestitureRequests.clear();
         releaseRequests.clear();
      }
   }
}
