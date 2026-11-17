package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
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

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      runInternal(factory, fom, objectClassName, attributeName, interactionName,
         parameterName, firstDimensionName, secondDimensionName, null, null, true, callbackModel);
   }

   static void runCustomTransportation(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, String transportationName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(transportationName != null && !transportationName.isEmpty(),
         "adapter did not supply a custom transportation name");
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "adapter did not supply a logical-time implementation");
      runInternal(factory, fom, null, null, interactionName, parameterName, firstDimensionName,
         secondDimensionName, timeImplementation, transportationName, false, callbackModel);
   }

   private static void runInternal(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName,
         String timeImplementation, String expectedTransportationName,
         boolean verifyRestrictedDefaultRange, CallbackModel callbackModel) throws Exception {
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
      boolean omittedDimensionSubscribed = false;
      InteractionClassHandle ownerInteraction = null;
      InteractionClassHandle overlapInteraction = null;
      InteractionClassHandle disjointInteraction = null;
      RegionHandle sourceRegion = null;
      RegionHandle overlapRegion = null;
      RegionHandle disjointRegion = null;
      RegionHandle omittedDimensionSourceRegion = null;
      RegionHandle omittedDimensionReceiverRegion = null;
      RegionHandleSet sourceRegions = null;
      RegionHandleSet overlapRegions = null;
      RegionHandleSet disjointRegions = null;
      RegionHandleSet omittedDimensionSourceRegions = null;
      RegionHandleSet omittedDimensionReceiverRegions = null;
      try {
         owner.connect(noopFederateAmbassador(), callbackModel);
         ownerConnected = true;
         overlap.connect(overlapRecorder.proxy(), callbackModel);
         overlapConnected = true;
         disjoint.connect(disjointRecorder.proxy(), callbackModel);
         disjointConnected = true;
         if (timeImplementation == null) {
            owner.createFederationExecution(federation, fom);
         } else {
            owner.createFederationExecution(federation, fom, timeImplementation);
         }
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-region-owner", federation);
         ownerJoined = true;
         FederateHandle overlapFederate = overlap.joinFederationExecution(
             "java-tck-region-overlap", federation);
         overlapJoined = true;
         FederateHandle disjointFederate = disjoint.joinFederationExecution(
             "java-tck-region-disjoint", federation);
         disjointJoined = true;

         if (verifyRestrictedDefaultRange) {
             JavaTckSupport.check(producer != null && producer.encodedLength() > 0
                   && overlapFederate != null && overlapFederate.encodedLength() > 0
                   && disjointFederate != null && disjointFederate.encodedLength() > 0,
                "regional interaction routing returned an invalid federate handle");
            verifyObjectClassDimensions(owner, overlap, disjoint, objectClassName,
               attributeName, firstDimensionName, secondDimensionName);
         }

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
               && ownerSecond.equals(overlapSecond) && ownerSecond.equals(disjointSecond)
               && !ownerFirst.equals(ownerSecond),
            "adapter FOM returned inconsistent interaction, parameter, or dimension handles");
          if (verifyRestrictedDefaultRange) {
             JavaTckSupport.check(ownerInteraction != null && ownerInteraction.encodedLength() > 0
                   && overlapInteraction != null && overlapInteraction.encodedLength() > 0
                   && disjointInteraction != null && disjointInteraction.encodedLength() > 0
                   && ownerParameter != null && ownerParameter.encodedLength() > 0
                   && overlapParameter != null && overlapParameter.encodedLength() > 0
                   && disjointParameter != null && disjointParameter.encodedLength() > 0
                   && ownerFirst != null && ownerFirst.encodedLength() > 0
                   && ownerSecond != null && ownerSecond.encodedLength() > 0
                   && overlapFirst != null && overlapFirst.encodedLength() > 0
                   && overlapSecond != null && overlapSecond.encodedLength() > 0
                   && disjointFirst != null && disjointFirst.encodedLength() > 0
                   && disjointSecond != null && disjointSecond.encodedLength() > 0,
                "regional interaction routing returned an invalid interaction, parameter, or dimension handle");
          }
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
         overlapRecorder.inspectCallbackRegions(overlap, overlapFirst, overlapSecond);
         disjointRecorder.inspectCallbackRegions(disjoint, disjointFirst, disjointSecond);

         owner.publishInteractionClass(ownerInteraction);
         published = true;
         if (verifyRestrictedDefaultRange) {
            verifyRegionTemplateLifecycle(owner, ownerInteraction, ownerParameter,
               ownerFirst, ownerSecond);
            verifySingleDimensionRegionSet(owner, ownerFirst, ownerSecond);
            omittedDimensionSourceRegion = createRegion(owner, ownerFirst, 1, 5, true);
            omittedDimensionReceiverRegion = createRegion(overlap, overlapSecond, 6, 9, true);
            omittedDimensionSourceRegions = regionSet(owner, omittedDimensionSourceRegion);
            omittedDimensionReceiverRegions = regionSet(overlap, omittedDimensionReceiverRegion);
            omittedDimensionSubscribed = true;
            verifyOmittedDimensionRouting(owner, overlap, overlapRecorder, ownerInteraction,
               overlapInteraction, ownerParameter, overlapParameter, producer,
               omittedDimensionSourceRegions, omittedDimensionReceiverRegion,
               omittedDimensionReceiverRegions, overlapSecond, callbackModel);
            verifySpecifiedDimensionBoundaries(owner, overlap, overlapRecorder,
               ownerInteraction, overlapInteraction, ownerParameter, overlapParameter, producer,
               omittedDimensionSourceRegions, overlapFirst, callbackModel);
            verifyModifiedSubscriptionDefaultRange(owner, overlap, overlapRecorder,
               ownerInteraction, overlapInteraction, ownerParameter, overlapParameter, producer,
               ownerFirst, ownerSecond, overlapFirst, callbackModel);
            omittedDimensionSubscribed = false;
            owner.deleteRegion(omittedDimensionSourceRegion);
            overlap.deleteRegion(omittedDimensionReceiverRegion);
            omittedDimensionSourceRegion = null;
            omittedDimensionReceiverRegion = null;
            omittedDimensionSourceRegions = null;
            omittedDimensionReceiverRegions = null;
            verifyFullRangeDefaultEquivalence(owner, overlap, overlapRecorder,
               ownerInteraction, overlapInteraction, ownerParameter, overlapParameter, producer,
               ownerFirst, ownerSecond, overlapFirst, overlapSecond, callbackModel);
         }
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 1, 5,
            verifyRestrictedDefaultRange);
         overlapRegion = createRegion(overlap, overlapFirst, overlapSecond, 2, 6, 2, 6,
            verifyRestrictedDefaultRange);
         disjointRegion = createRegion(disjoint, disjointFirst, disjointSecond, 7, 9, 7, 9,
            verifyRestrictedDefaultRange);
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
            firstValue, firstTag, overlapFirst, overlapSecond, expectedTransportationName);
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
            secondValue, secondTag, overlapFirst, overlapSecond, expectedTransportationName);
         pump(disjoint, callbackModel);
         JavaTckSupport.check(disjointRecorder.interactions.isEmpty(),
            "disjoint receiver received a later regional interaction");
      } finally {
         if (omittedDimensionSubscribed && overlapJoined) {
            try { overlap.unsubscribeInteractionClassWithRegions(
               overlapInteraction, omittedDimensionReceiverRegions); }
            catch (Exception ignored) { }
         }
         if (omittedDimensionSourceRegion != null && ownerJoined) {
            try { owner.deleteRegion(omittedDimensionSourceRegion); } catch (Exception ignored) { }
         }
         if (omittedDimensionReceiverRegion != null && overlapJoined) {
            try { overlap.deleteRegion(omittedDimensionReceiverRegion); } catch (Exception ignored) { }
         }
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

   private static void verifyObjectClassDimensions(RTIambassador owner, RTIambassador overlap,
         RTIambassador disjoint, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName) throws Exception {
      ObjectClassHandle ownerObject = owner.getObjectClassHandle(objectClassName);
      ObjectClassHandle overlapObject = overlap.getObjectClassHandle(objectClassName);
      ObjectClassHandle disjointObject = disjoint.getObjectClassHandle(objectClassName);
      AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerObject, attributeName);
      AttributeHandle overlapAttribute = overlap.getAttributeHandle(overlapObject, attributeName);
      AttributeHandle disjointAttribute = disjoint.getAttributeHandle(disjointObject, attributeName);
      DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
      DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
      DimensionHandle overlapFirst = overlap.getDimensionHandle(firstDimensionName);
      DimensionHandle overlapSecond = overlap.getDimensionHandle(secondDimensionName);
      DimensionHandle disjointFirst = disjoint.getDimensionHandle(firstDimensionName);
      DimensionHandle disjointSecond = disjoint.getDimensionHandle(secondDimensionName);
      JavaTckSupport.check(ownerObject != null && ownerObject.encodedLength() > 0
             && overlapObject != null && overlapObject.encodedLength() > 0
             && disjointObject != null && disjointObject.encodedLength() > 0
             && ownerAttribute != null && ownerAttribute.encodedLength() > 0
             && overlapAttribute != null && overlapAttribute.encodedLength() > 0
             && disjointAttribute != null && disjointAttribute.encodedLength() > 0
             && ownerFirst != null && ownerFirst.encodedLength() > 0
             && ownerSecond != null && ownerSecond.encodedLength() > 0
             && overlapFirst != null && overlapFirst.encodedLength() > 0
             && overlapSecond != null && overlapSecond.encodedLength() > 0
             && disjointFirst != null && disjointFirst.encodedLength() > 0
             && disjointSecond != null && disjointSecond.encodedLength() > 0,
          "adapter FOM lookup returned an invalid object, attribute, or dimension handle");
      JavaTckSupport.check(ownerObject.equals(overlapObject)
            && ownerObject.equals(disjointObject)
            && ownerAttribute.equals(overlapAttribute)
            && ownerAttribute.equals(disjointAttribute)
            && ownerFirst.equals(overlapFirst) && ownerFirst.equals(disjointFirst)
            && ownerSecond.equals(overlapSecond) && ownerSecond.equals(disjointSecond),
         "adapter FOM returned inconsistent object, attribute, or dimension handles");
      JavaTckSupport.check(owner.getAvailableDimensionsForObjectClass(ownerObject)
            .contains(ownerFirst)
            && owner.getAvailableDimensionsForObjectClass(ownerObject).contains(ownerSecond)
            && overlap.getAvailableDimensionsForObjectClass(overlapObject).contains(overlapFirst)
            && overlap.getAvailableDimensionsForObjectClass(overlapObject).contains(overlapSecond)
            && disjoint.getAvailableDimensionsForObjectClass(disjointObject).contains(disjointFirst)
            && disjoint.getAvailableDimensionsForObjectClass(disjointObject).contains(disjointSecond),
         "adapter object class does not expose both configured dimensions at every member");
      JavaTckSupport.check(firstDimensionName.equals(owner.getDimensionName(ownerFirst))
            && secondDimensionName.equals(owner.getDimensionName(ownerSecond))
            && firstDimensionName.equals(overlap.getDimensionName(overlapFirst))
            && secondDimensionName.equals(overlap.getDimensionName(overlapSecond))
            && firstDimensionName.equals(disjoint.getDimensionName(disjointFirst))
            && secondDimensionName.equals(disjoint.getDimensionName(disjointSecond)),
         "adapter dimension-name lookup returned a different configured name");
      JavaTckSupport.check(owner.getDimensionUpperBound(ownerFirst) >= 9
            && owner.getDimensionUpperBound(ownerSecond) >= 9,
         "adapter-supplied dimensions do not contain the routing test ranges");
   }

   private static void verifyRegionTemplateLifecycle(RTIambassador owner,
         InteractionClassHandle interaction, ParameterHandle parameter,
         DimensionHandle first, DimensionHandle second) throws Exception {
      DimensionHandleSet dimensions = owner.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle template = owner.createRegion(dimensions);
      checkRegionHandle(template, "region-template creation");
      RegionHandleSet regions = regionSet(owner, template);
      try {
         byte[] value = new byte[] {0x52};
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(parameter, value);
         byte[] tag = new byte[0];
         JavaTckSupport.expectFailure(() -> owner.sendInteractionWithRegions(
               interaction, parameters, regions, tag), "InvalidRegion",
            "sending an interaction before setting region-template bounds");
         owner.setRangeBounds(template, first, new RangeBounds(1, 5));
         JavaTckSupport.expectFailure(() -> owner.sendInteractionWithRegions(
               interaction, parameters, regions, tag), "InvalidRegion",
            "sending before setting every specified region dimension");
         owner.setRangeBounds(template, second, new RangeBounds(1, 5));
         JavaTckSupport.expectFailure(() -> owner.sendInteractionWithRegions(
               interaction, parameters, regions, tag), "InvalidRegion",
            "sending before committing region-template bounds");
         owner.commitRegionModifications(regions);
         owner.sendInteractionWithRegions(interaction, parameters, regions, tag);
      } finally {
         owner.deleteRegion(template);
      }
   }

   private static void verifySingleDimensionRegionSet(RTIambassador owner,
         DimensionHandle first, DimensionHandle second) throws Exception {
      DimensionHandleSet dimensions = owner.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      RegionHandle region = owner.createRegion(dimensions);
      checkRegionHandle(region, "single-dimension region creation");
      try {
         owner.setRangeBounds(region, first, new RangeBounds(1, 5));
         owner.commitRegionModifications(regionSet(owner, region));
         DimensionHandleSet actual = owner.getDimensionHandleSet(region);
         JavaTckSupport.check(actual.size() == 1 && actual.contains(first)
               && !actual.contains(second),
            "setting ranges and committing changed the region's Create Region dimension set");
      } finally {
         owner.deleteRegion(region);
      }
   }

   private static void verifyOmittedDimensionRouting(RTIambassador owner, RTIambassador overlap,
         Recorder recorder, InteractionClassHandle ownerInteraction,
         InteractionClassHandle overlapInteraction, ParameterHandle ownerParameter,
         ParameterHandle overlapParameter, FederateHandle producer,
         RegionHandleSet sourceRegions, RegionHandle receiverRegion,
         RegionHandleSet receiverRegions, DimensionHandle receiverDimension,
         CallbackModel callbackModel) throws Exception {
      RegionHandleSet emptyRegions = overlap.getRegionHandleSetFactory().create();
      overlap.subscribeInteractionClassWithRegions(overlapInteraction, emptyRegions);
      send(owner, ownerInteraction, ownerParameter, sourceRegions,
         new byte[] {0x72}, new byte[] {0x6F});
      awaitNoInteraction(overlap, recorder, callbackModel,
         "an empty regional interaction subscription created a delivery route");

      overlap.subscribeInteractionClassWithRegions(overlapInteraction, receiverRegions);
      send(owner, ownerInteraction, ownerParameter, sourceRegions,
         new byte[] {0x72}, new byte[] {0x70});
      awaitNoInteraction(overlap, recorder, callbackModel,
         "an omitted interaction-region dimension ignored its FOM default range");

      overlap.setRangeBounds(receiverRegion, receiverDimension, new RangeBounds(1, 4));
      send(owner, ownerInteraction, ownerParameter, sourceRegions,
         new byte[] {0x72}, new byte[] {0x76});
      awaitNoInteraction(overlap, recorder, callbackModel,
         "Set Range Bounds changed a committed interaction subscription before commit");
      overlap.commitRegionModifications(receiverRegions);
      send(owner, ownerInteraction, ownerParameter, sourceRegions,
         new byte[] {0x72}, new byte[] {0x71});
      assertProbeDelivery(overlap, recorder, callbackModel, overlapInteraction,
         overlapParameter, producer, new byte[] {0x72}, new byte[] {0x71},
         "delivery inside the omitted dimension's FOM default range");

      recorder.interactions.clear();
      overlap.unsubscribeInteractionClassWithRegions(overlapInteraction, emptyRegions);
      send(owner, ownerInteraction, ownerParameter, sourceRegions,
         new byte[] {0x72}, new byte[] {0x7A});
      assertProbeDelivery(overlap, recorder, callbackModel, overlapInteraction,
         overlapParameter, producer, new byte[] {0x72}, new byte[] {0x7A},
         "an empty regional unsubscription preserving the active route");

      recorder.interactions.clear();
      overlap.setRangeBounds(receiverRegion, receiverDimension, new RangeBounds(6, 9));
      send(owner, ownerInteraction, ownerParameter, sourceRegions,
         new byte[] {0x72}, new byte[] {0x77});
      assertProbeDelivery(overlap, recorder, callbackModel, overlapInteraction,
         overlapParameter, producer, new byte[] {0x72}, new byte[] {0x77},
         "the committed subscription bounds before region commit");

      recorder.interactions.clear();
      overlap.setRangeBounds(receiverRegion, receiverDimension, new RangeBounds(1, 4));
      overlap.commitRegionModifications(receiverRegions);
      send(owner, ownerInteraction, ownerParameter, sourceRegions,
         new byte[] {0x72}, new byte[] {0x78});
      assertProbeDelivery(overlap, recorder, callbackModel, overlapInteraction,
         overlapParameter, producer, new byte[] {0x72}, new byte[] {0x78},
         "the latest bounds committed for the regional subscription");

      overlap.unsubscribeInteractionClassWithRegions(overlapInteraction, receiverRegions);
      recorder.interactions.clear();
      send(owner, ownerInteraction, ownerParameter, sourceRegions,
         new byte[] {0x72}, new byte[] {0x79});
      awaitNoInteraction(overlap, recorder, callbackModel,
         "an unsubscribed interaction-region pair remained active for delivery");
   }

   private static void verifySpecifiedDimensionBoundaries(RTIambassador owner,
         RTIambassador overlap, Recorder recorder, InteractionClassHandle ownerInteraction,
         InteractionClassHandle overlapInteraction, ParameterHandle ownerParameter,
         ParameterHandle overlapParameter, FederateHandle producer, RegionHandleSet sourceRegions,
         DimensionHandle overlapFirst, CallbackModel callbackModel) throws Exception {
      DimensionHandleSet dimensions = overlap.getDimensionHandleSetFactory().create();
      dimensions.add(overlapFirst);
      RegionHandle receiverRegion = overlap.createRegion(dimensions);
      checkRegionHandle(receiverRegion, "boundary-probe region creation");
      RegionHandleSet receiverRegions = regionSet(overlap, receiverRegion);
      boolean subscribed = false;
      long[][] bounds = {{0, 1}, {1, 2}, {4, 5}, {5, 6}};
      byte[] tags = {0x72, 0x73, 0x74, 0x75};
      boolean[] expectedDelivery = {false, true, true, false};
      String[] descriptions = {
         "the lower outside boundary [0,1)",
         "the lower included boundary [1,2)",
         "the upper included boundary [4,5)",
         "the upper outside boundary [5,6)"
      };
      try {
         for (int index = 0; index < bounds.length; ++index) {
            recorder.interactions.clear();
            overlap.setRangeBounds(receiverRegion, overlapFirst,
               new RangeBounds(bounds[index][0], bounds[index][1]));
            overlap.commitRegionModifications(receiverRegions);
            overlap.subscribeInteractionClassWithRegions(overlapInteraction, receiverRegions);
            subscribed = true;
            try {
               send(owner, ownerInteraction, ownerParameter, sourceRegions,
                  new byte[] {0x72}, new byte[] {tags[index]});
               if (expectedDelivery[index]) {
                  assertProbeDelivery(overlap, recorder, callbackModel, overlapInteraction,
                     overlapParameter, producer, new byte[] {0x72},
                     new byte[] {tags[index]}, "regional delivery at " + descriptions[index]);
               } else {
                  awaitNoInteraction(overlap, recorder, callbackModel,
                     "regional interaction unexpectedly delivered at " + descriptions[index]);
               }
            } finally {
               overlap.unsubscribeInteractionClassWithRegions(overlapInteraction, receiverRegions);
               subscribed = false;
            }
         }
      } finally {
         if (subscribed) {
            try { overlap.unsubscribeInteractionClassWithRegions(overlapInteraction, receiverRegions); }
            catch (Exception ignored) { }
         }
         overlap.deleteRegion(receiverRegion);
      }
   }

   private static void verifyModifiedSubscriptionDefaultRange(RTIambassador owner,
         RTIambassador overlap, Recorder recorder, InteractionClassHandle ownerInteraction,
         InteractionClassHandle overlapInteraction, ParameterHandle ownerParameter,
         ParameterHandle overlapParameter, FederateHandle producer, DimensionHandle ownerFirst,
         DimensionHandle ownerSecond, DimensionHandle overlapFirst,
         CallbackModel callbackModel) throws Exception {
      DimensionHandleSet receiverDimensions = overlap.getDimensionHandleSetFactory().create();
      receiverDimensions.add(overlapFirst);
      RegionHandle receiverRegion = overlap.createRegion(receiverDimensions);
      checkRegionHandle(receiverRegion, "modified-subscription receiver region creation");
      RegionHandleSet receiverRegions = regionSet(overlap, receiverRegion);
      DimensionHandleSet sourceDimensions = owner.getDimensionHandleSetFactory().create();
      sourceDimensions.add(ownerFirst);
      sourceDimensions.add(ownerSecond);
      RegionHandle sourceRegion = owner.createRegion(sourceDimensions);
      checkRegionHandle(sourceRegion, "modified-subscription source region creation");
      RegionHandleSet sourceRegions = regionSet(owner, sourceRegion);
      boolean subscribed = false;
      try {
         overlap.setRangeBounds(receiverRegion, overlapFirst, new RangeBounds(1, 5));
         overlap.commitRegionModifications(receiverRegions);
         overlap.subscribeInteractionClassWithRegions(overlapInteraction, receiverRegions);
         subscribed = true;

         verifyModifiedSubscriptionOutcome(owner, overlap, recorder, ownerInteraction,
            overlapInteraction, ownerParameter, overlapParameter, producer,
            sourceRegion, sourceRegions, receiverRegions, ownerFirst, ownerSecond,
            2, 3, 6, 9, 0x80, false, "before changing the active subscription region",
            callbackModel);
         verifyModifiedSubscriptionOutcome(owner, overlap, recorder, ownerInteraction,
            overlapInteraction, ownerParameter, overlapParameter, producer,
            sourceRegion, sourceRegions, receiverRegions, ownerFirst, ownerSecond,
            2, 3, 1, 4, 0x81, true, "in the default Y range before modification",
            callbackModel);

         overlap.setRangeBounds(receiverRegion, overlapFirst, new RangeBounds(6, 9));
         overlap.commitRegionModifications(receiverRegions);
         verifyModifiedSubscriptionOutcome(owner, overlap, recorder, ownerInteraction,
            overlapInteraction, ownerParameter, overlapParameter, producer,
            sourceRegion, sourceRegions, receiverRegions, ownerFirst, ownerSecond,
            7, 8, 6, 9, 0x82, false, "after changing the active subscription region",
            callbackModel);
         verifyModifiedSubscriptionOutcome(owner, overlap, recorder, ownerInteraction,
            overlapInteraction, ownerParameter, overlapParameter, producer,
            sourceRegion, sourceRegions, receiverRegions, ownerFirst, ownerSecond,
            7, 8, 1, 4, 0x83, true, "in the default Y range after modification",
            callbackModel);
      } finally {
         if (subscribed) {
            try { overlap.unsubscribeInteractionClassWithRegions(overlapInteraction, receiverRegions); }
            catch (Exception ignored) { }
         }
         owner.deleteRegion(sourceRegion);
         overlap.deleteRegion(receiverRegion);
      }
   }

   private static void verifyModifiedSubscriptionOutcome(RTIambassador owner,
         RTIambassador overlap, Recorder recorder, InteractionClassHandle ownerInteraction,
         InteractionClassHandle overlapInteraction, ParameterHandle ownerParameter,
         ParameterHandle overlapParameter, FederateHandle producer, RegionHandle sourceRegion,
         RegionHandleSet sourceRegions, RegionHandleSet receiverRegions,
         DimensionHandle ownerFirst, DimensionHandle ownerSecond,
         long sourceXLower, long sourceXUpper, long sourceYLower, long sourceYUpper,
         int tag, boolean expectedDelivery, String description,
         CallbackModel callbackModel) throws Exception {
      owner.setRangeBounds(sourceRegion, ownerFirst, new RangeBounds(sourceXLower, sourceXUpper));
      owner.setRangeBounds(sourceRegion, ownerSecond, new RangeBounds(sourceYLower, sourceYUpper));
      owner.commitRegionModifications(sourceRegions);
      recorder.interactions.clear();
      send(owner, ownerInteraction, ownerParameter, sourceRegions,
         new byte[] {0x73}, new byte[] {(byte) tag});
      if (expectedDelivery) {
         assertProbeDelivery(overlap, recorder, callbackModel,
            overlapInteraction, overlapParameter, producer, new byte[] {0x73},
            new byte[] {(byte) tag}, description);
      } else {
         awaitNoInteraction(overlap, recorder, callbackModel,
            "an unspecified dimension's default range was lost " + description);
      }
   }

   private static void verifyFullRangeDefaultEquivalence(RTIambassador owner,
         RTIambassador overlap, Recorder recorder, InteractionClassHandle ownerInteraction,
         InteractionClassHandle overlapInteraction, ParameterHandle ownerParameter,
         ParameterHandle overlapParameter, FederateHandle producer, DimensionHandle ownerFirst,
         DimensionHandle ownerSecond, DimensionHandle overlapFirst, DimensionHandle overlapSecond,
         CallbackModel callbackModel) throws Exception {
      DimensionHandle ownerUnrelated = owner.getDimensionHandle("TckUnrelatedDimension");
      DimensionHandle overlapUnrelated = overlap.getDimensionHandle("TckUnrelatedDimension");
      JavaTckSupport.check(ownerUnrelated != null && ownerUnrelated.encodedLength() > 0
            && overlapUnrelated != null && overlapUnrelated.encodedLength() > 0
            && ownerUnrelated.equals(overlapUnrelated)
            && !ownerUnrelated.equals(ownerFirst) && !ownerUnrelated.equals(ownerSecond),
         "default-region equivalence FOM omitted or duplicated its unrelated dimension");

      DimensionHandleSet allDimensions = owner.getDimensionHandleSetFactory().create();
      allDimensions.add(ownerFirst);
      allDimensions.add(ownerSecond);
      allDimensions.add(ownerUnrelated);
      RegionHandle sourceRegion = owner.createRegion(allDimensions);
      checkRegionHandle(sourceRegion, "full-FDD source region creation");
      RegionHandleSet sourceRegions = regionSet(owner, sourceRegion);
      RegionHandle validSourceRegion = null;
      RegionHandleSet validSourceRegions = null;
      RegionHandle receiverRegion = null;
      RegionHandleSet receiverRegions = null;
      boolean subscribed = false;
      String stage = "creating and committing the full-range source region";
      try {
         for (DimensionHandle dimension : allDimensions) {
            long upper = owner.getDimensionUpperBound(dimension);
            JavaTckSupport.check(upper > 0,
               "default-region equivalence FOM has a non-positive dimension upper bound");
            owner.setRangeBounds(sourceRegion, dimension, new RangeBounds(0, upper));
         }
         owner.commitRegionModifications(sourceRegions);
         DimensionHandleSet committedDimensions = owner.getDimensionHandleSet(sourceRegion);
         JavaTckSupport.check(committedDimensions.size() == allDimensions.size()
               && committedDimensions.contains(ownerFirst)
               && committedDimensions.contains(ownerSecond)
               && committedDimensions.contains(ownerUnrelated),
            "the explicit full-FDD region did not retain every Create Region dimension");
         for (DimensionHandle dimension : allDimensions) {
            long upper = owner.getDimensionUpperBound(dimension);
            JavaTckSupport.check(boundsMatch(owner.getRangeBounds(sourceRegion, dimension), 0, upper),
               "the explicit FDD region did not retain a full [0, upperBound) range");
         }

         DimensionHandleSet ownerInteractionDimensions =
            owner.getAvailableDimensionsForInteractionClass(ownerInteraction);
         JavaTckSupport.check(ownerInteractionDimensions.size() == 2
               && ownerInteractionDimensions.contains(ownerFirst)
               && ownerInteractionDimensions.contains(ownerSecond)
               && !ownerInteractionDimensions.contains(ownerUnrelated),
            "the source interaction's FDD dimensions no longer match the regional routing FOM");
         DimensionHandleSet interactionDimensions =
            overlap.getAvailableDimensionsForInteractionClass(overlapInteraction);
         JavaTckSupport.check(interactionDimensions.size() == 2
               && interactionDimensions.contains(overlapFirst)
               && interactionDimensions.contains(overlapSecond)
               && !interactionDimensions.contains(overlapUnrelated),
            "the test FOM no longer isolates its unrelated third dimension");

         validSourceRegion = owner.createRegion(ownerInteractionDimensions);
         checkRegionHandle(validSourceRegion, "full-range interaction source region creation");
         validSourceRegions = regionSet(owner, validSourceRegion);
         for (DimensionHandle dimension : ownerInteractionDimensions) {
            owner.setRangeBounds(validSourceRegion, dimension,
               new RangeBounds(0, owner.getDimensionUpperBound(dimension)));
         }
         owner.commitRegionModifications(validSourceRegions);
         JavaTckSupport.check(owner.getDimensionHandleSet(validSourceRegion)
               .equals(ownerInteractionDimensions),
            "the valid interaction source region changed its Create Region dimension set");

         receiverRegion = overlap.createRegion(interactionDimensions);
         checkRegionHandle(receiverRegion, "full-range receiver region creation");
         receiverRegions = regionSet(overlap, receiverRegion);
         for (DimensionHandle dimension : interactionDimensions) {
            overlap.setRangeBounds(receiverRegion, dimension,
               new RangeBounds(0, overlap.getDimensionUpperBound(dimension)));
         }
         overlap.commitRegionModifications(receiverRegions);
         stage = "subscribing with the full-range interaction region";
         overlap.subscribeInteractionClassWithRegions(overlapInteraction, receiverRegions);
         subscribed = true;

         byte[] value = new byte[] {(byte) 0xA4};
         stage = "rejecting the explicit full-FDD source region with an unrelated dimension";
         JavaTckSupport.expectFailure(() -> send(owner, ownerInteraction, ownerParameter,
               sourceRegions, value, new byte[] {(byte) 0xAF}),
            "InvalidRegionContext",
            "sending with a full-FDD region containing a dimension unavailable to the interaction");
         awaitNoInteraction(overlap, recorder, callbackModel,
            "a region-context-rejected interaction created a partial delivery");
         owner.deleteRegion(sourceRegion);
         sourceRegion = null;

         stage = "sending the default-region control interaction";
         sendWithoutRegions(owner, ownerInteraction, ownerParameter, value, new byte[] {(byte) 0xA1});
         assertProbeDelivery(overlap, recorder, callbackModel, overlapInteraction,
            overlapParameter, producer, value, new byte[] {(byte) 0xA1},
            "default full-range interaction delivery");
         stage = "sending with the explicit full-range interaction-dimension source region";
         send(owner, ownerInteraction, ownerParameter, validSourceRegions,
            value, new byte[] {(byte) 0xA2});
         assertProbeDelivery(overlap, recorder, callbackModel, overlapInteraction,
            overlapParameter, producer, value, new byte[] {(byte) 0xA2},
            "explicit full-range interaction delivery");

         stage = "deleting the valid explicit source region and sending the default control";
         owner.deleteRegion(validSourceRegion);
         validSourceRegion = null;
         sendWithoutRegions(owner, ownerInteraction, ownerParameter, value, new byte[] {(byte) 0xA3});
         assertProbeDelivery(overlap, recorder, callbackModel, overlapInteraction,
            overlapParameter, producer, value, new byte[] {(byte) 0xA3},
            "default interaction delivery after deleting the explicit source region");
      } catch (Exception error) {
         throw new AssertionError("full-range equivalence failed while " + stage + ": "
            + error.getClass().getName() + ": " + error.getMessage(), error);
      } finally {
         if (subscribed && receiverRegions != null) {
            try { overlap.unsubscribeInteractionClassWithRegions(overlapInteraction, receiverRegions); }
            catch (Exception ignored) { }
         }
         if (sourceRegion != null) {
            try { owner.deleteRegion(sourceRegion); } catch (Exception ignored) { }
         }
         if (validSourceRegion != null) {
            try { owner.deleteRegion(validSourceRegion); } catch (Exception ignored) { }
         }
         if (receiverRegion != null) {
            try { overlap.deleteRegion(receiverRegion); } catch (Exception ignored) { }
         }
      }
   }

   private static void assertProbeDelivery(RTIambassador receiver, Recorder recorder,
         CallbackModel callbackModel, InteractionClassHandle interaction,
         ParameterHandle parameter, FederateHandle producer, byte[] value, byte[] tag,
         String description) throws Exception {
      await(callbackModel, receiver, () -> recorder.interactions.size() == 1, description);
      Interaction actual = recorder.interactions.get(0);
      JavaTckSupport.check(actual.interaction.equals(interaction)
            && actual.parameters.size() == 1
            && Arrays.equals(value, actual.parameters.get(parameter))
            && Arrays.equals(tag, actual.tag)
            && producer.equals(actual.producer),
         description + " returned the wrong interaction payload or metadata");
      recorder.interactions.clear();
   }

   private static void awaitNoInteraction(RTIambassador receiver, Recorder recorder,
         CallbackModel callbackModel, String failure) throws Exception {
      long deadline = System.nanoTime() + 250_000_000L;
      while (System.nanoTime() < deadline) {
         pump(receiver, callbackModel);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(recorder.interactions.isEmpty(), failure);
   }

   private static void send(RTIambassador ambassador, InteractionClassHandle interaction,
         ParameterHandle parameter, RegionHandleSet regions, byte[] value, byte[] tag)
         throws Exception {
      ParameterHandleValueMap parameters =
         ambassador.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value.clone());
      ambassador.sendInteractionWithRegions(interaction, parameters, regions, tag.clone());
   }

   private static void sendWithoutRegions(RTIambassador ambassador,
         InteractionClassHandle interaction, ParameterHandle parameter,
         byte[] value, byte[] tag) throws Exception {
      ParameterHandleValueMap parameters =
         ambassador.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value.clone());
      ambassador.sendInteraction(interaction, parameters, tag.clone());
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long firstLower, long firstUpper,
         long secondLower, long secondUpper, boolean verifyHandle) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      if (verifyHandle) checkRegionHandle(region, "regional interaction region creation");
      ambassador.setRangeBounds(region, first, new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, second, new RangeBounds(secondLower, secondUpper));
      RegionHandleSet regions = regionSet(ambassador, region);
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle dimension,
         long lower, long upper, boolean verifyHandle) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(dimension);
      RegionHandle region = ambassador.createRegion(dimensions);
      if (verifyHandle) checkRegionHandle(region, "single-dimension routing region creation");
      ambassador.setRangeBounds(region, dimension, new RangeBounds(lower, upper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static void checkRegionHandle(RegionHandle region, String description) {
      JavaTckSupport.check(region != null && region.encodedLength() > 0,
         description + " returned an invalid handle");
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
         DimensionHandle expectedFirst, DimensionHandle expectedSecond,
         String expectedTransportationName) throws Exception {
      boolean matchingClass = actual.interaction.equals(expectedClass);
      boolean matchingParameters = actual.parameters.size() == 1
         && Arrays.equals(expectedValue, actual.parameters.get(expectedParameter));
      boolean matchingTag = Arrays.equals(expectedTag, actual.tag);
      boolean matchingProducer = expectedProducer.equals(actual.producer);
      boolean matchingRegions = actual.regions != null && actual.regions.size() == 1
          && (expectedTransportationName != null
             || actual.regions.iterator().next().encodedLength() > 0)
         && actual.regionDimensions != null && actual.regionDimensions.size() == 2
         && actual.regionDimensions.contains(expectedFirst)
         && actual.regionDimensions.contains(expectedSecond)
         && boundsMatch(actual.firstRegionBounds, 1, 5)
         && boundsMatch(actual.secondRegionBounds, 1, 5);
      boolean matchingTransportation = actual.transportation != null
          && (expectedTransportationName != null || actual.transportation.encodedLength() > 0);
      JavaTckSupport.check(matchingClass && matchingParameters && matchingTag
            && matchingProducer && matchingTransportation && matchingRegions,
         "regional interaction metadata mismatch: class=" + matchingClass
            + ", parameters=" + matchingParameters + ", tag=" + matchingTag
            + ", producer=" + matchingProducer + ", transportation="
            + (actual.transportation != null) + ", regions=" + matchingRegions
            + " (actual handles " + actual.regions + ", dimensions "
            + actual.regionDimensions + ", bounds " + actual.firstRegionBounds + ", "
            + actual.secondRegionBounds + ")");
      String actualTransportationName = receiver.getTransportationTypeName(actual.transportation);
      JavaTckSupport.check(!actualTransportationName.isEmpty()
            && (expectedTransportationName == null
               || expectedTransportationName.equals(actualTransportationName)),
         "regional interaction returned an unresolvable transportation type");
   }

   private static boolean boundsMatch(RangeBounds actual, long lower, long upper) {
      return actual != null && actual.lower == lower && actual.upper == upper;
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
      private final Set<DimensionHandle> regionDimensions;
      private final RangeBounds firstRegionBounds;
      private final RangeBounds secondRegionBounds;

      private Interaction(Object[] args, RTIambassador regionInspector,
            DimensionHandle firstDimension, DimensionHandle secondDimension) throws Exception {
         interaction = (InteractionClassHandle) args[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) args[1];
         values.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) args[2]).clone();
         transportation = (TransportationTypeHandle) args[3];
         producer = (FederateHandle) args[4];
         regions = args[5] == null ? null : new HashSet<>((RegionHandleSet) args[5]);
         if (regions == null || regions.size() != 1 || regionInspector == null
               || firstDimension == null || secondDimension == null) {
            regionDimensions = null;
            firstRegionBounds = null;
            secondRegionBounds = null;
         } else {
            RegionHandle callbackRegion = regions.iterator().next();
            regionDimensions = new HashSet<>(regionInspector.getDimensionHandleSet(callbackRegion));
            firstRegionBounds = regionInspector.getRangeBounds(callbackRegion, firstDimension);
            secondRegionBounds = regionInspector.getRangeBounds(callbackRegion, secondDimension);
         }
      }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private volatile RTIambassador regionInspector;
      private volatile DimensionHandle firstDimension;
      private volatile DimensionHandle secondDimension;

      private void inspectCallbackRegions(RTIambassador inspector, DimensionHandle first,
            DimensionHandle second) {
         regionInspector = inspector;
         firstDimension = first;
         secondDimension = second;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               if ("receiveInteraction".equals(method.getName())
                     && args != null && args.length == 6) {
                  interactions.add(new Interaction(args, regionInspector,
                     firstDimension, secondDimension));
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
