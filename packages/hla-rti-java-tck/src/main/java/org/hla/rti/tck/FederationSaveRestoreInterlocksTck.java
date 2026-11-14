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
import hla.rti1516_2025.FederateHandleSet;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Declaration, directed interaction, ownership, time, order, transportation, advisory,
 * synchronization, and regional services are interlocked. */
final class FederationSaveRestoreInterlocksTck {
   private FederationSaveRestoreInterlocksTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel, String firstDimensionName,
         String secondDimensionName) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      String federation = "java-tck-save-restore-interlocks-" + UUID.randomUUID();
      String ownerName = "java-tck-save-restore-interlocks-owner";
      String peerName = "java-tck-save-restore-interlocks-peer";
      boolean ownerConnected = false, peerConnected = false, created = false;
      boolean ownerJoined = false, peerJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         peer.connect(peerRecorder.proxy(), callbackModel);
         peerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(ownerName, federation);
         ownerJoined = true;
         peer.joinFederationExecution(peerName, federation);
         peerJoined = true;

         ObjectClassHandle ownerObjectClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle peerObjectClass = peer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerObjectClass, attributeName);
         AttributeHandle peerAttribute = peer.getAttributeHandle(peerObjectClass, attributeName);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle peerInteraction =
            peer.getInteractionClassHandle(interactionClassName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         InteractionClassHandleSet ownerDirectedInteractions =
            JavaTckSupport.interactionSet(owner, ownerInteraction);
         InteractionClassHandleSet peerDirectedInteractions =
            JavaTckSupport.interactionSet(peer, peerInteraction);
         TransportationTypeHandle reliable = owner.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null && reliable.encodedLength() > 0,
            "save/restore interlock setup returned an invalid transportation handle");
         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         LogicalTime<?, ?> interlockTime = timeFactory.makeInitial();
         LogicalTimeInterval<?> interlockLookahead = timeFactory.makeEpsilon();
         JavaTckSupport.check(interlockTime != null && interlockLookahead != null,
            "save/restore interlock setup returned incomplete logical-time values");

         AttributeHandleSet ownerAttributes =
            JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet peerAttributes =
            JavaTckSupport.attributeSet(peer, peerAttribute);
         owner.publishObjectClassAttributes(ownerObjectClass, ownerAttributes);
         peer.subscribeObjectClassAttributes(peerObjectClass, peerAttributes);
         owner.publishInteractionClass(ownerInteraction);
         peer.subscribeInteractionClass(peerInteraction);
         owner.publishObjectClassDirectedInteractions(ownerObjectClass, ownerDirectedInteractions);
         peer.subscribeObjectClassDirectedInteractions(peerObjectClass, peerDirectedInteractions);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerObjectClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "save/restore interlock setup returned an invalid object handle");
         DimensionHandle firstDimension = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle secondDimension = owner.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(firstDimension != null && secondDimension != null
               && !firstDimension.equals(secondDimension),
            "save/restore interlock setup returned invalid or duplicate DDM dimensions");
         DimensionHandleSet dimensions = owner.getDimensionHandleSetFactory().create();
         dimensions.add(firstDimension);
         dimensions.add(secondDimension);
         RegionHandle region = owner.createRegion(dimensions);
         owner.setRangeBounds(region, firstDimension, new RangeBounds(1L, 5L));
         owner.setRangeBounds(region, secondDimension, new RangeBounds(1L, 5L));
         RegionHandleSet regions = owner.getRegionHandleSetFactory().create();
         regions.add(region);
         owner.commitRegionModifications(regions);
         AttributeSetRegionSetPairList attributeRegions =
            owner.getAttributeSetRegionSetPairListFactory().create(1);
         attributeRegions.add(new AttributeRegionAssociation(ownerAttributes, regions));
         FederateHandleSet synchronizationSet = owner.getFederateHandleSetFactory().create();
         synchronizationSet.add(ownerFederate);

         AttributeHandleValueMap attributeValues = owner.getAttributeHandleValueMapFactory()
            .create(1);
         attributeValues.put(ownerAttribute, new byte[] {0x41, 0x42});
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, new byte[] {0x53, 0x52});

         String saveLabel = "java-tck-save-restore-interlocks-save";
         owner.requestFederationSave(saveLabel);
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.saveInitiations.size() == 1
               && peerRecorder.saveInitiations.size() == 1,
            "save initiation callbacks");
         JavaTckSupport.check(saveLabel.equals(ownerRecorder.saveInitiations.get(0))
               && saveLabel.equals(peerRecorder.saveInitiations.get(0)),
            "save interlock setup received the wrong save label");
         owner.federateSaveBegun();
         peer.federateSaveBegun();
         assertInterlocks(owner, peer, ownerObjectClass, peerObjectClass, ownerAttributes,
            peerAttributes, ownerInteraction, peerInteraction, ownerDirectedInteractions,
            peerDirectedInteractions, object, dimensions, ownerAttribute, ownerFederate, reliable,
            regions, attributeRegions, synchronizationSet, attributeValues,
            parameters, interlockTime,
            interlockLookahead, saveLabel, "SaveInProgress");
         owner.federateSaveComplete();
         peer.federateSaveComplete();
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.savedCount == 1 && peerRecorder.savedCount == 1,
            "save completion callbacks");
         JavaTckSupport.check(ownerRecorder.saveFailureCount == 0
               && peerRecorder.saveFailureCount == 0,
            "successful save reported a failure");

         owner.requestFederationRestore(saveLabel);
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.restoreSuccessCount == 1
               && ownerRecorder.restoreInitiationCount == 1
               && peerRecorder.restoreInitiationCount == 1
               && ownerRecorder.restoreBegunCount == 1
               && peerRecorder.restoreBegunCount == 1,
            "restore initiation callbacks");
         assertInterlocks(owner, peer, ownerObjectClass, peerObjectClass, ownerAttributes,
            peerAttributes, ownerInteraction, peerInteraction, ownerDirectedInteractions,
            peerDirectedInteractions, object, dimensions, ownerAttribute, ownerFederate, reliable,
            regions, attributeRegions, synchronizationSet, attributeValues,
            parameters, interlockTime,
            interlockLookahead, saveLabel, "RestoreInProgress");
         owner.federateRestoreComplete();
         peer.federateRestoreComplete();
         await(owner, peer, callbackModel, objectClassName,
            () -> ownerRecorder.restoredCount == 1 && peerRecorder.restoredCount == 1,
            "restore completion callbacks");
         JavaTckSupport.check(ownerRecorder.restoreFailureCount == 0
               && peerRecorder.restoreFailureCount == 0,
            "successful restore reported a failure");
      } finally {
         if (peerJoined) resign(peer, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (peerConnected) disconnect(peer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void assertInterlocks(RTIambassador owner, RTIambassador peer,
         ObjectClassHandle ownerObjectClass, ObjectClassHandle peerObjectClass,
         AttributeHandleSet ownerAttributes, AttributeHandleSet peerAttributes,
         InteractionClassHandle ownerInteraction, InteractionClassHandle peerInteraction,
         InteractionClassHandleSet ownerDirectedInteractions,
         InteractionClassHandleSet peerDirectedInteractions, ObjectInstanceHandle object,
         DimensionHandleSet dimensions, AttributeHandle ownerAttribute,
         FederateHandle ownerFederate,
         TransportationTypeHandle reliable, RegionHandleSet regions,
         AttributeSetRegionSetPairList attributeRegions,
         FederateHandleSet synchronizationSet,
         AttributeHandleValueMap attributeValues, ParameterHandleValueMap parameters,
         LogicalTime<?, ?> interlockTime,
         LogicalTimeInterval<?> interlockLookahead, String saveLabel, String expected)
         throws Exception {
      JavaTckSupport.expectFailure(() -> owner.requestFederationRestore(saveLabel), expected,
         "requesting another federation restore during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.publishObjectClassAttributes(ownerObjectClass, ownerAttributes), expected,
         "publishing object attributes during " + expected);
      JavaTckSupport.expectFailure(() -> owner.unpublishObjectClass(ownerObjectClass), expected,
         "unpublishing an object class during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.unpublishObjectClassAttributes(ownerObjectClass, ownerAttributes), expected,
         "unpublishing object attributes during " + expected);
      JavaTckSupport.expectFailure(
         () -> peer.subscribeObjectClassAttributes(peerObjectClass, peerAttributes),
         expected, "subscribing to object attributes during " + expected);
      JavaTckSupport.expectFailure(() -> peer.unsubscribeObjectClass(peerObjectClass), expected,
         "unsubscribing an object class during " + expected);
      JavaTckSupport.expectFailure(
         () -> peer.unsubscribeObjectClassAttributes(peerObjectClass, peerAttributes), expected,
         "unsubscribing object attributes during " + expected);
      JavaTckSupport.expectFailure(() -> owner.publishInteractionClass(ownerInteraction), expected,
         "publishing an interaction during " + expected);
      JavaTckSupport.expectFailure(() -> owner.unpublishInteractionClass(ownerInteraction),
         expected, "unpublishing an interaction during " + expected);
      JavaTckSupport.expectFailure(() -> peer.subscribeInteractionClass(peerInteraction),
         expected, "subscribing to an interaction during " + expected);
      JavaTckSupport.expectFailure(() -> peer.unsubscribeInteractionClass(peerInteraction),
         expected, "unsubscribing an interaction during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.publishObjectClassDirectedInteractions(ownerObjectClass,
            ownerDirectedInteractions), expected,
         "publishing directed interactions during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.unpublishObjectClassDirectedInteractions(ownerObjectClass), expected,
         "unpublishing all directed interactions during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.unpublishObjectClassDirectedInteractions(ownerObjectClass,
            ownerDirectedInteractions), expected,
         "unpublishing selected directed interactions during " + expected);
      JavaTckSupport.expectFailure(
         () -> peer.subscribeObjectClassDirectedInteractions(peerObjectClass,
            peerDirectedInteractions), expected,
         "subscribing to directed interactions during " + expected);
      JavaTckSupport.expectFailure(
         () -> peer.unsubscribeObjectClassDirectedInteractions(peerObjectClass), expected,
         "unsubscribing all directed interactions during " + expected);
      JavaTckSupport.expectFailure(
         () -> peer.unsubscribeObjectClassDirectedInteractions(peerObjectClass,
            peerDirectedInteractions), expected,
         "unsubscribing selected directed interactions during " + expected);
      JavaTckSupport.expectFailure(() -> owner.registerObjectInstance(ownerObjectClass), expected,
         "registering an object instance during " + expected);
      JavaTckSupport.expectFailure(() -> owner.updateAttributeValues(object, attributeValues,
         new byte[] {0x49}), expected, "updating attribute values during " + expected);
      JavaTckSupport.expectFailure(() -> owner.sendInteraction(ownerInteraction, parameters,
         new byte[] {0x49}), expected, "sending an interaction during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.changeAttributeOrderType(object, ownerAttributes, OrderType.RECEIVE),
         expected, "changing object-attribute order during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.changeDefaultAttributeOrderType(ownerObjectClass, ownerAttributes,
            OrderType.RECEIVE), expected,
         "changing default object-attribute order during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.changeInteractionOrderType(ownerInteraction, OrderType.RECEIVE), expected,
         "changing interaction order during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.requestAttributeTransportationTypeChange(object, ownerAttributes, reliable),
         expected, "requesting an attribute transportation change during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.changeDefaultAttributeTransportationType(ownerObjectClass, ownerAttributes,
            reliable), expected, "changing default attribute transportation during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.queryAttributeTransportationType(object, ownerAttribute), expected,
         "querying attribute transportation during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.requestInteractionTransportationTypeChange(ownerInteraction, reliable),
         expected, "requesting an interaction transportation change during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.queryInteractionTransportationType(ownerFederate, ownerInteraction),
         expected, "querying interaction transportation during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.queryAttributeOwnership(object, ownerAttributes), expected,
         "querying attribute ownership during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.isAttributeOwnedByFederate(object, ownerAttributes.iterator().next()),
         expected, "querying ownership by federate during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes,
            new byte[0]), expected, "requesting negotiated divestiture during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.confirmDivestiture(object, ownerAttributes, new byte[0]), expected,
         "confirming divestiture during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.cancelNegotiatedAttributeOwnershipDivestiture(object, ownerAttributes),
         expected, "cancelling negotiated divestiture during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.unconditionalAttributeOwnershipDivestiture(object, ownerAttributes,
            new byte[0]), expected, "requesting unconditional divestiture during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.attributeOwnershipReleaseDenied(object, ownerAttributes, new byte[0]),
         expected, "reporting denied ownership release during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.attributeOwnershipDivestitureIfWanted(object, ownerAttributes, new byte[0]),
         expected, "requesting conditional divestiture during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.cancelAttributeOwnershipAcquisition(object, ownerAttributes), expected,
         "cancelling ownership acquisition during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.attributeOwnershipAcquisition(object, ownerAttributes, new byte[0]),
         expected, "acquiring attribute ownership during " + expected);
      JavaTckSupport.expectFailure(() -> owner.createRegion(dimensions), expected,
         "creating a region during " + expected);
      JavaTckSupport.expectFailure(() -> owner.timeAdvanceRequest(interlockTime), expected,
         "requesting a time advance during " + expected);
      JavaTckSupport.expectFailure(() -> owner.enableTimeRegulation(interlockLookahead), expected,
         "enabling time regulation during " + expected);
      JavaTckSupport.expectFailure(owner::disableTimeRegulation, expected,
         "disabling time regulation during " + expected);
      JavaTckSupport.expectFailure(owner::enableTimeConstrained, expected,
         "enabling time constrained during " + expected);
      JavaTckSupport.expectFailure(owner::disableTimeConstrained, expected,
         "disabling time constrained during " + expected);
      JavaTckSupport.expectFailure(owner::queryLogicalTime, expected,
         "querying logical time during " + expected);
      JavaTckSupport.expectFailure(owner::queryGALT, expected,
         "querying GALT during " + expected);
      JavaTckSupport.expectFailure(owner::queryLITS, expected,
         "querying LITS during " + expected);
      JavaTckSupport.expectFailure(owner::queryLookahead, expected,
         "querying lookahead during " + expected);
      JavaTckSupport.expectFailure(() -> owner.modifyLookahead(interlockLookahead), expected,
         "modifying lookahead during " + expected);
      JavaTckSupport.expectFailure(owner::getAttributeScopeAdvisorySwitch, expected,
         "getting the attribute-scope advisory switch during " + expected);
      JavaTckSupport.expectFailure(() -> owner.setAttributeScopeAdvisorySwitch(false), expected,
         "setting the attribute-scope advisory switch during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.registerFederationSynchronizationPoint("tck-interlock-sync", new byte[0]),
         expected, "registering a synchronization point during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.registerFederationSynchronizationPoint("tck-interlock-sync-set",
            new byte[0], synchronizationSet), expected,
         "registering an explicit-set synchronization point during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.synchronizationPointAchieved("tck-interlock-sync"), expected,
         "achieving a synchronization point during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.registerObjectInstanceWithRegions(ownerObjectClass, attributeRegions),
         expected, "registering an object instance with regions during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.associateRegionsForUpdates(object, attributeRegions), expected,
         "associating regions for updates during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.unassociateRegionsForUpdates(object, attributeRegions), expected,
         "unassociating regions for updates during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.subscribeObjectClassAttributesWithRegions(ownerObjectClass,
            attributeRegions), expected,
         "subscribing to regional object attributes during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.unsubscribeObjectClassAttributesWithRegions(ownerObjectClass,
            attributeRegions), expected,
         "unsubscribing regional object attributes during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.requestAttributeValueUpdateWithRegions(ownerObjectClass, attributeRegions,
            new byte[0]), expected,
         "requesting regional attribute values during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.subscribeInteractionClassWithRegions(ownerInteraction, regions), expected,
         "subscribing to regional interactions during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.unsubscribeInteractionClassWithRegions(ownerInteraction, regions), expected,
         "unsubscribing regional interactions during " + expected);
      JavaTckSupport.expectFailure(
         () -> owner.sendInteractionWithRegions(ownerInteraction, parameters, regions,
            new byte[0]), expected,
         "sending a regional interaction during " + expected);
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(first, model, objectClassName);
         service(second, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface Condition { boolean isTrue() throws Exception; }

   private static final class Recorder {
      private final CopyOnWriteArrayList<String> saveInitiations = new CopyOnWriteArrayList<>();
      private volatile int saveFailureCount;
      private volatile int savedCount;
      private volatile int restoreSuccessCount;
      private volatile int restoreBegunCount;
      private volatile int restoreInitiationCount;
      private volatile int restoreFailureCount;
      private volatile int restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)) saveInitiations.add((String) arguments[0]);
               else if ("federationSaved".equals(name)) savedCount++;
               else if ("federationNotSaved".equals(name)) saveFailureCount++;
               else if ("requestFederationRestoreSucceeded".equals(name)) restoreSuccessCount++;
               else if ("federationRestoreBegun".equals(name)) restoreBegunCount++;
               else if ("initiateFederateRestore".equals(name)) restoreInitiationCount++;
               else if ("federationNotRestored".equals(name)) restoreFailureCount++;
               else if ("federationRestored".equals(name)) restoredCount++;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK save/restore interlock recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
