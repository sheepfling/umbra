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
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

/** Verifies ownership transfer clears the former owner's regional update source. */
final class OwnershipTransferRegionalUpdateTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private OwnershipTransferRegionalUpdateTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, String unrelatedDimensionName,
         CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied dimensional FOM is required");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador newOwner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder newOwnerCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-ownership-transfer-regional-update-" + UUID.randomUUID();
      boolean[] connected = new boolean[3];
      boolean created = false;
      boolean[] joined = new boolean[3];
      boolean ownerPublished = false;
      boolean newOwnerPublished = false;
      boolean newOwnerSubscribed = false;
      boolean receiverSubscribed = false;
      RegionHandle ownerRegion = null;
      RegionHandle newOwnerRegion = null;
      RegionHandle receiverOwnerRegion = null;
      RegionHandle receiverReplacementRegion = null;
      RegionHandle invalidRegion = null;
      AttributeSetRegionSetPairList ownerPair = null;
      AttributeSetRegionSetPairList newOwnerPair = null;
      AttributeSetRegionSetPairList receiverPair = null;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle newOwnerClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle newOwnerAttribute = null;
      AttributeHandle receiverAttribute = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet newOwnerAttributes = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected[0] = true;
         newOwner.connect(newOwnerCallbacks.proxy(), callbackModel);
         connected[1] = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         connected[2] = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle ownerFederate =
            owner.joinFederationExecution("regional-update-owner", federation);
         joined[0] = true;
         FederateHandle newOwnerFederate =
            newOwner.joinFederationExecution("regional-update-new-owner", federation);
         joined[1] = true;
         receiver.joinFederationExecution("regional-update-receiver", federation);
         joined[2] = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         newOwnerClass = newOwner.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         newOwnerAttribute = newOwner.getAttributeHandle(newOwnerClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         DimensionHandle[] ownerDimensions = dimensions(owner, firstDimensionName, secondDimensionName);
         DimensionHandle[] newOwnerDimensions = dimensions(newOwner, firstDimensionName, secondDimensionName);
         DimensionHandle[] receiverDimensions = dimensions(receiver, firstDimensionName, secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(newOwnerClass) && ownerClass.equals(receiverClass)
               && ownerAttribute.equals(newOwnerAttribute)
               && ownerAttribute.equals(receiverAttribute)
               && ownerDimensions[0].equals(newOwnerDimensions[0])
               && ownerDimensions[1].equals(newOwnerDimensions[1])
               && ownerDimensions[0].equals(receiverDimensions[0])
               && ownerDimensions[1].equals(receiverDimensions[1]),
            "ownership/DDM adapter FOM resolved inconsistent class, attribute, or dimensions");
         checkObjectDimensions(owner, ownerClass, ownerDimensions,
            firstDimensionName, secondDimensionName);
         receiverCallbacks.configureReflectionInspection(receiver, receiverDimensions);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         newOwnerAttributes = JavaTckSupport.attributeSet(newOwner, newOwnerAttribute);
         AttributeHandleSet receiverAttributes =
            JavaTckSupport.attributeSet(receiver, receiverAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         newOwner.publishObjectClassAttributes(newOwnerClass, newOwnerAttributes);
         newOwnerPublished = true;

         ownerRegion = createRegion(owner, ownerDimensions, 1, 3, 1, 3);
         newOwnerRegion = createRegion(newOwner, newOwnerDimensions, 1, 3, 1, 3);
         receiverOwnerRegion = createRegion(receiver, receiverDimensions, 1, 3, 1, 3);
         receiverReplacementRegion = createRegion(receiver, receiverDimensions, 6, 8, 6, 8);
         ownerPair = pairList(owner, ownerAttributes, ownerRegion);
         DimensionHandle unrelatedDimension = owner.getDimensionHandle(unrelatedDimensionName);
         DimensionHandleSet availableDimensions =
            owner.getAvailableDimensionsForObjectClass(ownerClass);
         JavaTckSupport.check(!availableDimensions.contains(unrelatedDimension),
            "unrelated DDM dimension unexpectedly belongs to the object class");
         DimensionHandleSet unrelatedDimensionSet =
            owner.getDimensionHandleSetFactory().create();
         unrelatedDimensionSet.add(unrelatedDimension);
         invalidRegion = owner.createRegion(unrelatedDimensionSet);
         owner.setRangeBounds(invalidRegion, unrelatedDimension, new RangeBounds(1, 3));
         commitRegion(owner, invalidRegion);
         expectInvalidRegionContext(owner, ownerClass, ownerAttributes, invalidRegion);
         owner.deleteRegion(invalidRegion);
         invalidRegion = null;
         newOwnerPair = pairList(newOwner, newOwnerAttributes, newOwnerRegion);
         receiverPair = pairList(receiver, receiverAttributes,
            receiverOwnerRegion, receiverReplacementRegion);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "receiver did not enable region-designator callbacks");
         receiver.setAttributeScopeAdvisorySwitch(true);
         JavaTckSupport.check(receiver.getAttributeScopeAdvisorySwitch(),
            "receiver did not enable attribute-scope advisories");
         newOwner.subscribeObjectClassAttributesWithRegions(newOwnerClass, newOwnerPair);
         newOwnerSubscribed = true;
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         receiverSubscribed = true;

         object = owner.registerObjectInstanceWithRegions(ownerClass, ownerPair);
         JavaTckSupport.check(object != null,
            "ownership-transfer regional registration returned a null object handle");
         ObjectInstanceHandle registeredObject = object;
         awaitAll(owner, newOwner, receiver, callbackModel,
            () -> newOwnerCallbacks.discoveries.contains(registeredObject)
               && receiverCallbacks.discoveries.contains(registeredObject),
            "ownership-transfer regional discovery");

         byte[] ownerValue = new byte[] {0x4f, 0x4c, 0x44, 0x25};
         byte[] defaultValue = new byte[] {0x44, 0x45, 0x46, 0x25};
         byte[] replacementValue = new byte[] {0x4e, 0x45, 0x57, 0x25};
         byte[] acquisitionTag = new byte[] {0x41, 0x43, 0x51, 0x25};
         byte[] divestitureTag = new byte[] {0x44, 0x49, 0x56, 0x25};

         update(owner, object, ownerAttribute, ownerValue, divestitureTag);
         await(receiver, callbackModel, () -> !receiverCallbacks.reflections.isEmpty(),
            "initial regional owner reflection");
         receiverCallbacks.checkCallbackFailures();
         assertReflection(receiverCallbacks.reflections.get(0), object, receiverAttribute,
            ownerFederate, ownerValue, divestitureTag,
            new RegionBoundsSnapshot(1, 3, 1, 3), receiver,
            "initial owner reflection");

         receiverCallbacks.clearScopeAdvisories();
         newOwnerCallbacks.acquisitions.clear();
         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            // In immediate delivery, If Available reports unavailable inline;
            // use a regular pending acquisition to reach the same transfer point.
            newOwner.attributeOwnershipAcquisition(object, newOwnerAttributes, acquisitionTag);
         } else {
            newOwner.attributeOwnershipAcquisitionIfAvailable(
               object, newOwnerAttributes, acquisitionTag);
         }
         JavaTckSupport.check(newOwnerCallbacks.acquisitions.isEmpty(),
            "ownership acquisition completed before the owner divested");
         AttributeHandleSet divested = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerAttributes, divestitureTag);
         JavaTckSupport.check(divested != null && divested.equals(ownerAttributes),
            "If Wanted divestiture returned the wrong attribute set");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && newOwner.isAttributeOwnedByFederate(object, newOwnerAttribute),
            "ownership transfer did not move the attribute to the pending acquirer");
         await(newOwner, callbackModel, () -> !newOwnerCallbacks.acquisitions.isEmpty(),
            "ownership acquisition notification");
         OwnershipEvent acquisition = newOwnerCallbacks.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquisition.attributes.contains(newOwnerAttribute)
               && Arrays.equals(divestitureTag, acquisition.tag),
            "ownership acquisition callback returned the wrong object, attributes, or tag");
         settle(receiver, callbackModel);
         receiverCallbacks.requireNoScopeAdvisories(
            "Divestiture If Wanted caused a transient attribute scope transition");

         receiverCallbacks.reflections.clear();
         receiver.setRangeBounds(receiverOwnerRegion, receiverDimensions[0],
            new RangeBounds(7, 9));
         receiver.setRangeBounds(receiverOwnerRegion, receiverDimensions[1],
            new RangeBounds(7, 9));
         commitRegion(receiver, receiverOwnerRegion);
         expectAttributeNotOwned(owner, object, ownerAttribute, divestitureTag);
         update(newOwner, object, newOwnerAttribute, defaultValue, divestitureTag);
         await(receiver, callbackModel, () -> !receiverCallbacks.reflections.isEmpty(),
            "new-owner default-source reflection");
         receiverCallbacks.checkCallbackFailures();
         assertReflection(receiverCallbacks.reflections.get(0), object, receiverAttribute,
            newOwnerFederate, defaultValue, divestitureTag, null, receiver,
            "new-owner default-source reflection");
         JavaTckSupport.check(receiverCallbacks.callbacksWithoutSourceRegions.get() == 1,
            "new-owner default-source reflection conveyed a source-region copy");

         receiverCallbacks.reflections.clear();
         newOwner.setRangeBounds(newOwnerRegion, newOwnerDimensions[0], new RangeBounds(6, 8));
         newOwner.setRangeBounds(newOwnerRegion, newOwnerDimensions[1], new RangeBounds(6, 8));
         commitRegion(newOwner, newOwnerRegion);
         newOwner.associateRegionsForUpdates(object, newOwnerPair);
         update(newOwner, object, newOwnerAttribute, replacementValue, acquisitionTag);
         await(receiver, callbackModel, () -> !receiverCallbacks.reflections.isEmpty(),
            "explicit replacement-region reflection");
         receiverCallbacks.checkCallbackFailures();
         assertReflection(receiverCallbacks.reflections.get(0), object, receiverAttribute,
            newOwnerFederate, replacementValue, acquisitionTag,
            new RegionBoundsSnapshot(6, 8, 6, 8),
            receiver, "explicit replacement-region reflection");

         ownerCallbacks.acquisitions.clear();
         newOwnerCallbacks.releaseRequests.clear();
         owner.attributeOwnershipAcquisition(object, ownerAttributes, acquisitionTag);
         await(newOwner, callbackModel, () -> !newOwnerCallbacks.releaseRequests.isEmpty(),
            "former-owner reacquisition release request");
         OwnershipEvent releaseRequest = newOwnerCallbacks.releaseRequests.get(0);
         JavaTckSupport.check(object.equals(releaseRequest.object)
               && releaseRequest.attributes.equals(newOwnerAttributes)
               && Arrays.equals(acquisitionTag, releaseRequest.tag),
            "former-owner reacquisition release request returned the wrong data");
         AttributeHandleSet reacquired = newOwner.attributeOwnershipDivestitureIfWanted(
            object, newOwnerAttributes, divestitureTag);
         JavaTckSupport.check(reacquired != null && reacquired.equals(newOwnerAttributes)
               && owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !newOwner.isAttributeOwnedByFederate(object, newOwnerAttribute),
            "former-owner reacquisition did not transfer ownership back");
         await(owner, callbackModel, () -> !ownerCallbacks.acquisitions.isEmpty(),
            "former-owner reacquisition notification");
         OwnershipEvent reacquisition = ownerCallbacks.acquisitions.get(0);
         JavaTckSupport.check(object.equals(reacquisition.object)
               && reacquisition.attributes.equals(ownerAttributes)
               && Arrays.equals(divestitureTag, reacquisition.tag),
            "former-owner reacquisition notification returned the wrong data");

         byte[] reacquiredDefaultValue = new byte[] {0x52, 0x45, 0x41, 0x43};
         receiverCallbacks.reflections.clear();
         update(owner, object, ownerAttribute, reacquiredDefaultValue, acquisitionTag);
         await(receiver, callbackModel, () -> !receiverCallbacks.reflections.isEmpty(),
            "reacquired owner default-source reflection");
         receiverCallbacks.checkCallbackFailures();
         assertReflection(receiverCallbacks.reflections.get(0), object, receiverAttribute,
            ownerFederate, reacquiredDefaultValue, acquisitionTag, null, receiver,
            "reacquired owner default-source reflection");
         JavaTckSupport.check(receiverCallbacks.callbacksWithoutSourceRegions.get() == 2,
            "reacquired owner did not use the default source region");

         receiver.setRangeBounds(receiverOwnerRegion, receiverDimensions[0],
            new RangeBounds(1, 3));
         receiver.setRangeBounds(receiverOwnerRegion, receiverDimensions[1],
            new RangeBounds(1, 3));
         commitRegion(receiver, receiverOwnerRegion);
         owner.associateRegionsForUpdates(object, ownerPair);
         byte[] reassociatedValue = new byte[] {0x52, 0x45, 0x41, 0x53};
         receiverCallbacks.reflections.clear();
         update(owner, object, ownerAttribute, reassociatedValue, divestitureTag);
         await(receiver, callbackModel, () -> !receiverCallbacks.reflections.isEmpty(),
            "explicitly reassociated former-owner reflection");
         receiverCallbacks.checkCallbackFailures();
         assertReflection(receiverCallbacks.reflections.get(0), object, receiverAttribute,
            ownerFederate, reassociatedValue, divestitureTag,
            new RegionBoundsSnapshot(1, 3, 1, 3), receiver,
            "explicitly reassociated former-owner reflection");
         JavaTckSupport.check(receiverCallbacks.callbacksWithoutSourceRegions.get() == 2,
            "explicit reassociation changed the default-source callback count");

         receiverCallbacks.clearScopeAdvisories();
         ownerCallbacks.releaseRequests.clear();
         ownerCallbacks.divestitureConfirmations.clear();
         newOwnerCallbacks.acquisitions.clear();
         owner.negotiatedAttributeOwnershipDivestiture(
            object, ownerAttributes, divestitureTag);
         newOwner.attributeOwnershipAcquisition(object, newOwnerAttributes, acquisitionTag);
         await(owner, callbackModel, () -> !ownerCallbacks.divestitureConfirmations.isEmpty(),
            "negotiated divestiture confirmation request");
         OwnershipEvent confirmationRequest = ownerCallbacks.divestitureConfirmations.get(0);
         JavaTckSupport.check(object.equals(confirmationRequest.object)
               && confirmationRequest.attributes.equals(ownerAttributes)
               && Arrays.equals(acquisitionTag, confirmationRequest.tag)
               && owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !newOwner.isAttributeOwnedByFederate(object, newOwnerAttribute),
            "negotiated divestiture confirmation request returned the wrong data or ownership");
         settle(receiver, callbackModel);
         receiverCallbacks.requireNoScopeAdvisories(
            "pending negotiated divestiture caused a transient attribute scope transition");
         owner.confirmDivestiture(object, ownerAttributes, divestitureTag);
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && newOwner.isAttributeOwnedByFederate(object, newOwnerAttribute),
            "confirmed negotiated divestiture did not transfer ownership immediately");
         await(newOwner, callbackModel, () -> !newOwnerCallbacks.acquisitions.isEmpty(),
            "negotiated acquisition notification");
         OwnershipEvent negotiatedAcquisition = newOwnerCallbacks.acquisitions.get(0);
         JavaTckSupport.check(object.equals(negotiatedAcquisition.object)
               && negotiatedAcquisition.attributes.equals(newOwnerAttributes)
               && Arrays.equals(divestitureTag, negotiatedAcquisition.tag),
            "negotiated acquisition notification returned the wrong data");
         settle(receiver, callbackModel);
         receiverCallbacks.requireNoScopeAdvisories(
            "confirmed negotiated divestiture caused a transient attribute scope transition");

         newOwner.associateRegionsForUpdates(object, newOwnerPair);
         receiverCallbacks.clearScopeAdvisories();
         receiver.setRangeBounds(receiverOwnerRegion, receiverDimensions[0],
            new RangeBounds(8, 10));
         receiver.setRangeBounds(receiverOwnerRegion, receiverDimensions[1],
            new RangeBounds(8, 10));
         receiver.setRangeBounds(receiverReplacementRegion, receiverDimensions[0],
            new RangeBounds(0, 1));
         receiver.setRangeBounds(receiverReplacementRegion, receiverDimensions[1],
            new RangeBounds(0, 1));
         RegionHandleSet regionsToCommit = receiver.getRegionHandleSetFactory().create();
         regionsToCommit.add(receiverOwnerRegion);
         regionsToCommit.add(receiverReplacementRegion);
         receiver.commitRegionModifications(regionsToCommit);
         await(receiver, callbackModel, () -> !receiverCallbacks.attributesOutOfScope.isEmpty(),
            "live out-of-scope advisory control");
         requireSingleScopeAdvisory(receiverCallbacks.attributesOutOfScope, object,
            receiverAttributes, "live out-of-scope advisory control");
         JavaTckSupport.check(receiverCallbacks.attributesInScope.isEmpty(),
            "out-of-scope control also produced an in-scope advisory");
         receiverCallbacks.clearScopeAdvisories();
         receiver.setRangeBounds(receiverReplacementRegion, receiverDimensions[0],
            new RangeBounds(6, 8));
         receiver.setRangeBounds(receiverReplacementRegion, receiverDimensions[1],
            new RangeBounds(6, 8));
         commitRegion(receiver, receiverReplacementRegion);
         await(receiver, callbackModel, () -> !receiverCallbacks.attributesInScope.isEmpty(),
            "live in-scope advisory control");
         requireSingleScopeAdvisory(receiverCallbacks.attributesInScope, object,
            receiverAttributes, "live in-scope advisory control");
         JavaTckSupport.check(receiverCallbacks.attributesOutOfScope.isEmpty(),
            "in-scope control also produced an out-of-scope advisory");
         receiver.setAttributeScopeAdvisorySwitch(false);
         JavaTckSupport.check(!receiver.getAttributeScopeAdvisorySwitch(),
            "receiver did not disable attribute-scope advisories after the control");
      } finally {
         if (joined[2] && receiverSubscribed && receiverClass != null && receiverPair != null) {
            try { receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair); }
            catch (Exception ignored) { }
         }
         if (joined[1] && newOwnerSubscribed && newOwnerClass != null && newOwnerPair != null) {
            try { newOwner.unsubscribeObjectClassAttributesWithRegions(newOwnerClass, newOwnerPair); }
            catch (Exception ignored) { }
         }
         if (joined[1] && object != null && newOwnerPair != null) {
            try { newOwner.unassociateRegionsForUpdates(object, newOwnerPair); }
            catch (Exception ignored) { }
         }
         if (joined[0] && object != null && ownerPair != null) {
            try { owner.unassociateRegionsForUpdates(object, ownerPair); }
            catch (Exception ignored) { }
         }
         if (joined[0] && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (joined[1] && newOwnerPublished && newOwnerClass != null && newOwnerAttributes != null) {
            try { newOwner.unpublishObjectClassAttributes(newOwnerClass, newOwnerAttributes); }
            catch (Exception ignored) { }
         }
         if (joined[2] && receiverOwnerRegion != null) {
            try { receiver.deleteRegion(receiverOwnerRegion); } catch (Exception ignored) { }
         }
         if (joined[2] && receiverReplacementRegion != null) {
            try { receiver.deleteRegion(receiverReplacementRegion); } catch (Exception ignored) { }
         }
         if (joined[0] && invalidRegion != null) {
            try { owner.deleteRegion(invalidRegion); } catch (Exception ignored) { }
         }
         if (joined[1] && newOwnerRegion != null) {
            try { newOwner.deleteRegion(newOwnerRegion); } catch (Exception ignored) { }
         }
         if (joined[0] && ownerRegion != null) {
            try { owner.deleteRegion(ownerRegion); } catch (Exception ignored) { }
         }
         if (joined[2]) resign(receiver, ResignAction.NO_ACTION);
         if (joined[1]) resign(newOwner, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (joined[0]) resign(owner, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected[2]) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (connected[1]) try { newOwner.disconnect(); } catch (Exception ignored) { }
         if (connected[0]) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   static void runDeferred(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied dimensional FOM is required");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador newOwner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder newOwnerCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-ownership-transfer-deferred-regional-update-" + UUID.randomUUID();
      boolean[] connected = new boolean[3];
      boolean created = false;
      boolean[] joined = new boolean[3];
      boolean[] published = new boolean[2];
      boolean newOwnerSubscribed = false;
      boolean receiverSubscribed = false;
      RegionHandle ownerRegion = null;
      RegionHandle deferredRegion = null;
      RegionHandle receiverOwnerRegion = null;
      RegionHandle receiverDeferredRegion = null;
      AttributeSetRegionSetPairList ownerPair = null;
      AttributeSetRegionSetPairList deferredPair = null;
      AttributeSetRegionSetPairList receiverPair = null;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle newOwnerClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle newOwnerAttribute = null;
      AttributeHandle receiverAttribute = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet newOwnerAttributes = null;
      ObjectInstanceHandle object = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected[0] = true;
         newOwner.connect(newOwnerCallbacks.proxy(), callbackModel);
         connected[1] = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         connected[2] = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle ownerFederate =
            owner.joinFederationExecution("deferred-regional-owner", federation);
         joined[0] = true;
         FederateHandle newOwnerFederate =
            newOwner.joinFederationExecution("deferred-regional-new-owner", federation);
         joined[1] = true;
         receiver.joinFederationExecution("deferred-regional-receiver", federation);
         joined[2] = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         newOwnerClass = newOwner.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         newOwnerAttribute = newOwner.getAttributeHandle(newOwnerClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         DimensionHandle[] ownerDimensions = dimensions(owner, firstDimensionName, secondDimensionName);
         DimensionHandle[] deferredDimensions = dimensions(newOwner, firstDimensionName, secondDimensionName);
         DimensionHandle[] receiverDimensions = dimensions(receiver, firstDimensionName, secondDimensionName);
         JavaTckSupport.check(ownerClass.equals(newOwnerClass) && ownerClass.equals(receiverClass)
               && ownerAttribute.equals(newOwnerAttribute)
               && ownerAttribute.equals(receiverAttribute)
               && ownerDimensions[0].equals(deferredDimensions[0])
               && ownerDimensions[1].equals(deferredDimensions[1])
               && ownerDimensions[0].equals(receiverDimensions[0])
               && ownerDimensions[1].equals(receiverDimensions[1]),
            "deferred ownership/DDM FOM resolved inconsistent handles");
         checkObjectDimensions(owner, ownerClass, ownerDimensions,
            firstDimensionName, secondDimensionName);
         receiverCallbacks.configureReflectionInspection(receiver, receiverDimensions);

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         newOwnerAttributes = JavaTckSupport.attributeSet(newOwner, newOwnerAttribute);
         AttributeHandleSet receiverAttributes =
            JavaTckSupport.attributeSet(receiver, receiverAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published[0] = true;
         newOwner.publishObjectClassAttributes(newOwnerClass, newOwnerAttributes);
         published[1] = true;

         ownerRegion = createRegion(owner, ownerDimensions, 1, 3, 1, 3);
         deferredRegion = createRegion(newOwner, deferredDimensions, 1, 3, 1, 3);
         receiverOwnerRegion = createRegion(receiver, receiverDimensions, 1, 3, 1, 3);
         receiverDeferredRegion = createRegion(receiver, receiverDimensions, 1, 3, 1, 3);
         ownerPair = pairList(owner, ownerAttributes, ownerRegion);
         deferredPair = pairList(newOwner, newOwnerAttributes, deferredRegion);
         receiverPair = pairList(receiver, receiverAttributes,
            receiverOwnerRegion, receiverDeferredRegion);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         newOwner.subscribeObjectClassAttributesWithRegions(newOwnerClass, deferredPair);
         newOwnerSubscribed = true;
         receiver.subscribeObjectClassAttributesWithRegions(receiverClass, receiverPair);
         receiverSubscribed = true;
         object = owner.registerObjectInstanceWithRegions(ownerClass, ownerPair);
         ObjectInstanceHandle registeredObject = object;
         awaitAll(owner, newOwner, receiver, callbackModel,
            () -> newOwnerCallbacks.discoveries.contains(registeredObject)
               && receiverCallbacks.discoveries.contains(registeredObject),
            "deferred ownership-transfer discovery");

         // Give the pending owner's association a distinct realization so
         // callback-visible copies distinguish it from the current owner's region.
         newOwner.setRangeBounds(deferredRegion, deferredDimensions[0],
            new RangeBounds(6, 8));
         newOwner.setRangeBounds(deferredRegion, deferredDimensions[1],
            new RangeBounds(6, 8));
         commitRegion(newOwner, deferredRegion);
         receiver.setRangeBounds(receiverDeferredRegion, receiverDimensions[0],
            new RangeBounds(6, 8));
         receiver.setRangeBounds(receiverDeferredRegion, receiverDimensions[1],
            new RangeBounds(6, 8));
         commitRegion(receiver, receiverDeferredRegion);

         byte[] ownerValue = new byte[] {0x4f, 0x4c, 0x44, 0x26};
         byte[] newOwnerValue = new byte[] {0x4e, 0x45, 0x57, 0x26};
         byte[] acquisitionTag = new byte[] {0x41, 0x43, 0x51, 0x26};
         byte[] divestitureTag = new byte[] {0x44, 0x49, 0x56, 0x26};

         // A non-owner may stage the future source region; it must not redirect
         // the current owner's update before acquisition transfers the attribute.
         newOwner.associateRegionsForUpdates(object, deferredPair);
         update(owner, object, ownerAttribute, ownerValue, divestitureTag);
         await(receiver, callbackModel, () -> !receiverCallbacks.reflections.isEmpty(),
            "pre-transfer owner reflection");
         receiverCallbacks.checkCallbackFailures();
         assertReflection(receiverCallbacks.reflections.get(0), object, receiverAttribute,
            ownerFederate, ownerValue, divestitureTag,
            new RegionBoundsSnapshot(1, 3, 1, 3), receiver,
            "pre-transfer owner reflection");

         ownerCallbacks.releaseRequests.clear();
         newOwnerCallbacks.acquisitions.clear();
         receiverCallbacks.reflections.clear();
         newOwner.attributeOwnershipAcquisition(object, newOwnerAttributes, acquisitionTag);
         await(owner, callbackModel, () -> !ownerCallbacks.releaseRequests.isEmpty(),
            "deferred ownership release request");
         OwnershipEvent release = ownerCallbacks.releaseRequests.get(0);
         JavaTckSupport.check(object.equals(release.object)
               && release.attributes.contains(ownerAttribute)
               && Arrays.equals(acquisitionTag, release.tag),
            "deferred ownership release request returned the wrong metadata");
         AttributeHandleSet divested = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerAttributes, divestitureTag);
         JavaTckSupport.check(divested != null && divested.equals(ownerAttributes)
               && !owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && newOwner.isAttributeOwnedByFederate(object, newOwnerAttribute),
            "deferred ownership acquisition did not transfer the requested attribute");
         await(newOwner, callbackModel, () -> !newOwnerCallbacks.acquisitions.isEmpty(),
            "deferred ownership acquisition notification");
         OwnershipEvent acquisition = newOwnerCallbacks.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquisition.attributes.contains(newOwnerAttribute)
               && Arrays.equals(divestitureTag, acquisition.tag),
            "deferred ownership acquisition callback returned the wrong metadata");
         expectAttributeNotOwned(owner, object, ownerAttribute, divestitureTag);

         // No second association call: acquisition promotes the staged region.
         update(newOwner, object, newOwnerAttribute, newOwnerValue, acquisitionTag);
         await(receiver, callbackModel, () -> !receiverCallbacks.reflections.isEmpty(),
            "post-transfer deferred-region reflection");
         receiverCallbacks.checkCallbackFailures();
         assertReflection(receiverCallbacks.reflections.get(0), object, receiverAttribute,
            newOwnerFederate, newOwnerValue, acquisitionTag,
            new RegionBoundsSnapshot(6, 8, 6, 8), receiver,
            "post-transfer deferred-region reflection");
      } finally {
         if (joined[2] && receiverSubscribed && receiverClass != null && receiverPair != null) {
            try { receiver.unsubscribeObjectClassAttributesWithRegions(receiverClass, receiverPair); }
            catch (Exception ignored) { }
         }
         if (joined[1] && newOwnerSubscribed && newOwnerClass != null && deferredPair != null) {
            try { newOwner.unsubscribeObjectClassAttributesWithRegions(newOwnerClass, deferredPair); }
            catch (Exception ignored) { }
         }
         if (joined[1] && object != null && deferredPair != null) {
            try { newOwner.unassociateRegionsForUpdates(object, deferredPair); }
            catch (Exception ignored) { }
         }
         if (joined[0] && published[0] && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (joined[1] && published[1] && newOwnerClass != null && newOwnerAttributes != null) {
            try { newOwner.unpublishObjectClassAttributes(newOwnerClass, newOwnerAttributes); }
            catch (Exception ignored) { }
         }
         if (joined[2] && receiverOwnerRegion != null) {
            try { receiver.deleteRegion(receiverOwnerRegion); } catch (Exception ignored) { }
         }
         if (joined[2] && receiverDeferredRegion != null) {
            try { receiver.deleteRegion(receiverDeferredRegion); } catch (Exception ignored) { }
         }
         if (joined[1] && deferredRegion != null) {
            try { newOwner.deleteRegion(deferredRegion); } catch (Exception ignored) { }
         }
         if (joined[0] && ownerRegion != null) {
            try { owner.deleteRegion(ownerRegion); } catch (Exception ignored) { }
         }
         if (joined[2]) resign(receiver, ResignAction.NO_ACTION);
         if (joined[1]) resign(newOwner, ResignAction.NO_ACTION);
         if (joined[0]) resign(owner, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected[2]) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (connected[1]) try { newOwner.disconnect(); } catch (Exception ignored) { }
         if (connected[0]) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static DimensionHandle[] dimensions(RTIambassador ambassador, String firstName,
         String secondName) throws Exception {
      DimensionHandle first = ambassador.getDimensionHandle(firstName);
      DimensionHandle second = ambassador.getDimensionHandle(secondName);
      JavaTckSupport.check(!first.equals(second), "DDM FOM configured duplicate dimensions");
      return new DimensionHandle[] {first, second};
   }

   private static void checkObjectDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle[] dimensions, String firstName, String secondName) throws Exception {
      DimensionHandleSet available = ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(available.contains(dimensions[0]) && available.contains(dimensions[1])
            && ambassador.getDimensionName(dimensions[0]).equals(firstName)
            && ambassador.getDimensionName(dimensions[1]).equals(secondName)
            && ambassador.getDimensionUpperBound(dimensions[0]) >= 8
            && ambassador.getDimensionUpperBound(dimensions[1]) >= 8,
         "ownership/DDM FOM omitted an adapter dimension or required range");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle[] dimensions,
         long firstLower, long firstUpper, long secondLower, long secondUpper) throws Exception {
      DimensionHandleSet dimensionSet = ambassador.getDimensionHandleSetFactory().create();
      dimensionSet.add(dimensions[0]);
      dimensionSet.add(dimensions[1]);
      RegionHandle region = ambassador.createRegion(dimensionSet);
      ambassador.setRangeBounds(region, dimensions[0], new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, dimensions[1], new RangeBounds(secondLower, secondUpper));
      commitRegion(ambassador, region);
      return region;
   }

   private static AttributeSetRegionSetPairList pairList(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle... regions) throws Exception {
      RegionHandleSet regionSet = ambassador.getRegionHandleSetFactory().create();
      for (RegionHandle region : regions) regionSet.add(region);
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      pairs.add(new AttributeRegionAssociation(attributes, regionSet));
      return pairs;
   }

   private static void commitRegion(RTIambassador ambassador, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
   }

   private static void update(RTIambassador ambassador, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      ambassador.updateAttributeValues(object, values, tag.clone());
   }

   private static void expectAttributeNotOwned(RTIambassador ambassador,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] tag) throws Exception {
      try {
         update(ambassador, object, attribute, new byte[] {0x45, 0x52, 0x52}, tag);
      } catch (Exception exception) {
         JavaTckSupport.check("AttributeNotOwned".equals(exception.getClass().getSimpleName()),
            "former-owner update failed with the wrong exception: " + exception.getClass().getName());
         return;
      }
      JavaTckSupport.check(false, "former owner successfully updated an attribute after transfer");
   }

   private static void expectInvalidRegionContext(RTIambassador ambassador,
         ObjectClassHandle objectClass, AttributeHandleSet attributes, RegionHandle region)
         throws Exception {
      try {
         ambassador.registerObjectInstanceWithRegions(
            objectClass, pairList(ambassador, attributes, region));
      } catch (Exception exception) {
         JavaTckSupport.check("InvalidRegionContext".equals(
               exception.getClass().getSimpleName()),
            "registration with an unrelated dimension failed with the wrong exception: "
               + exception.getClass().getName());
         return;
      }
      JavaTckSupport.check(false,
         "registration accepted a source region with a dimension unavailable to the object class");
   }

   private static void assertReflection(Reflection reflection, ObjectInstanceHandle object,
         AttributeHandle attribute, FederateHandle producer, byte[] value, byte[] tag,
         RegionBoundsSnapshot expectedRegionBounds, RTIambassador receiver,
         String description) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer),
         description + " returned incorrect object, value, tag, or producer metadata");
      JavaTckSupport.check(reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty(),
         description + " omitted transportation metadata");
      if (expectedRegionBounds == null) {
         JavaTckSupport.check(reflection.regions == null || reflection.regions.isEmpty(),
            description + " inherited a former owner's source-region association");
      } else {
         JavaTckSupport.check(reflection.regions != null && reflection.regions.size() == 1
               && reflection.copiedRegionBounds.size() == 1
               && expectedRegionBounds.matches(reflection.copiedRegionBounds.get(0)),
            description + " returned the wrong copied source-region dimensions or bounds");
      }
   }

   private static void requireSingleScopeAdvisory(List<ScopeAdvisory> reports,
         ObjectInstanceHandle object, AttributeHandleSet attributes, String description) {
      JavaTckSupport.check(reports.size() == 1,
         description + " returned an unexpected callback count");
      ScopeAdvisory report = reports.get(0);
      JavaTckSupport.check(object.equals(report.object) && attributes.equals(report.attributes),
         description + " returned the wrong object or attribute set");
   }

   private static void awaitAll(RTIambassador first, RTIambassador second, RTIambassador third,
         CallbackModel callbackModel, java.util.function.BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.01);
            second.evokeCallback(0.01);
            third.evokeCallback(0.01);
         } else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         while (ambassador.evokeMultipleCallbacks(0.0, 0.0)) { }
      } else {
         long deadline = System.nanoTime() + 100_000_000L;
         while (System.nanoTime() < deadline) Thread.yield();
      }
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;
      private OwnershipEvent(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         attributes = (AttributeHandleSet) arguments[1];
         tag = ((byte[]) arguments[2]).clone();
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final List<RegionBoundsSnapshot> copiedRegionBounds = new java.util.ArrayList<>();
      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap received = (AttributeHandleValueMap) arguments[1];
         received.forEach((handle, bytes) -> values.put(handle, bytes.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         RegionHandleSet receivedRegions = (RegionHandleSet) arguments[5];
         regions = receivedRegions == null ? null : (RegionHandleSet) receivedRegions.clone();
      }
   }

   private static final class RegionBoundsSnapshot {
      private final long firstLower;
      private final long firstUpper;
      private final long secondLower;
      private final long secondUpper;
      private RegionBoundsSnapshot(long firstLower, long firstUpper,
            long secondLower, long secondUpper) {
         this.firstLower = firstLower;
         this.firstUpper = firstUpper;
         this.secondLower = secondLower;
         this.secondUpper = secondUpper;
      }
      private boolean matches(RegionBoundsSnapshot other) {
         return other != null && firstLower == other.firstLower && firstUpper == other.firstUpper
            && secondLower == other.secondLower && secondUpper == other.secondUpper;
      }
   }

   private static final class ScopeAdvisory {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private ScopeAdvisory(ObjectInstanceHandle object, AttributeHandleSet attributes) {
         this.object = object;
         this.attributes = (AttributeHandleSet) attributes.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> divestitureConfirmations = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<ScopeAdvisory> attributesInScope = new CopyOnWriteArrayList<>();
      private final List<ScopeAdvisory> attributesOutOfScope = new CopyOnWriteArrayList<>();
      private final List<Throwable> callbackFailures = new CopyOnWriteArrayList<>();
      private final AtomicInteger callbacksWithoutSourceRegions = new AtomicInteger();
      private volatile RTIambassador reflectionAmbassador;
      private volatile DimensionHandle[] reflectionDimensions;

      private void configureReflectionInspection(RTIambassador ambassador,
            DimensionHandle[] dimensions) {
         reflectionAmbassador = ambassador;
         reflectionDimensions = dimensions.clone();
      }

      private void checkCallbackFailures() {
         JavaTckSupport.check(callbackFailures.isEmpty(),
            "reflection callback inspection failed: "
               + (callbackFailures.isEmpty() ? "" : callbackFailures.get(0).toString()));
      }

      private void clearScopeAdvisories() {
         attributesInScope.clear();
         attributesOutOfScope.clear();
      }

      private void requireNoScopeAdvisories(String description) {
         JavaTckSupport.check(attributesInScope.isEmpty() && attributesOutOfScope.isEmpty(),
            description);
      }

      private void recordReflection(Object[] arguments) {
         Reflection reflection = new Reflection(arguments);
         try {
            if (reflection.regions == null || reflection.regions.isEmpty()) {
               callbacksWithoutSourceRegions.incrementAndGet();
            } else {
               JavaTckSupport.check(reflection.regions.size() == 1,
                  "ownership-transfer reflection conveyed an unexpected number of region copies");
               RTIambassador ambassador = reflectionAmbassador;
               DimensionHandle[] dimensions = reflectionDimensions;
               JavaTckSupport.check(ambassador != null && dimensions != null
                     && dimensions.length == 2,
                  "reflection callback region inspection was not configured");
               RegionHandle callbackRegion = reflection.regions.iterator().next();
               DimensionHandleSet expectedDimensions =
                  ambassador.getDimensionHandleSetFactory().create();
               expectedDimensions.add(dimensions[0]);
               expectedDimensions.add(dimensions[1]);
               JavaTckSupport.check(ambassador.getDimensionHandleSet(callbackRegion)
                     .equals(expectedDimensions),
                  "ownership-transfer callback region copy changed its dimension set");
               RangeBounds first = ambassador.getRangeBounds(callbackRegion, dimensions[0]);
               RangeBounds second = ambassador.getRangeBounds(callbackRegion, dimensions[1]);
               reflection.copiedRegionBounds.add(new RegionBoundsSnapshot(
                  first.lower, first.upper, second.lower, second.upper));
            }
         } catch (Throwable failure) {
            callbackFailures.add(failure);
         }
         reflections.add(reflection);
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  acquisitions.add(new OwnershipEvent(arguments));
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  releaseRequests.add(new OwnershipEvent(arguments));
               } else if ("requestDivestitureConfirmation".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  divestitureConfirmations.add(new OwnershipEvent(arguments));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  recordReflection(arguments);
               } else if ("attributesInScope".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  attributesInScope.add(new ScopeAdvisory(
                     (ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               } else if ("attributesOutOfScope".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  attributesOutOfScope.add(new ScopeAdvisory(
                     (ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional ownership callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
