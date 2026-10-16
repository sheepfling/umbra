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
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import hla.rti1516_2025.RangeBounds;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Standard timestamped regional-interaction delivery, filtering, and retraction checks. */
final class TimestampedRegionalInteractionTck {
   private TimestampedRegionalInteractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador immediate = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      RTIambassador disjoint = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder immediateRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      Recorder disjointRecorder = new Recorder();
      String federation = "java-tck-timestamped-regional-interaction-" + UUID.randomUUID();
      boolean ownerConnected = false, immediateConnected = false;
      boolean constrainedConnected = false, disjointConnected = false;
      boolean created = false, ownerJoined = false, immediateJoined = false;
      boolean constrainedJoined = false, disjointJoined = false;
      boolean published = false, immediateSubscribed = false;
      boolean constrainedSubscribed = false, disjointSubscribed = false;
      boolean regulating = false, timeConstrained = false;
      InteractionClassHandle ownerInteraction = null;
      InteractionClassHandle immediateInteraction = null;
      InteractionClassHandle constrainedInteraction = null;
      InteractionClassHandle disjointInteraction = null;
      RegionHandle sourceRegion = null, immediateRegion = null;
      RegionHandle constrainedRegion = null, disjointRegion = null;
      RegionHandleSet sourceRegions = null, immediateRegions = null;
      RegionHandleSet constrainedRegions = null, disjointRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         immediate.connect(immediateRecorder.proxy(), callbackModel);
         immediateConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;
         disjoint.connect(disjointRecorder.proxy(), callbackModel);
         disjointConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-tso-region-owner", federation);
         ownerJoined = true;
         immediate.joinFederationExecution("java-tck-tso-region-immediate", federation);
         immediateJoined = true;
         constrained.joinFederationExecution("java-tck-tso-region-constrained", federation);
         constrainedJoined = true;
         disjoint.joinFederationExecution("java-tck-tso-region-disjoint", federation);
         disjointJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         immediateInteraction = immediate.getInteractionClassHandle(interactionName);
         constrainedInteraction = constrained.getInteractionClassHandle(interactionName);
         disjointInteraction = disjoint.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle immediateParameter =
            immediate.getParameterHandle(immediateInteraction, parameterName);
         ParameterHandle constrainedParameter =
            constrained.getParameterHandle(constrainedInteraction, parameterName);
         ParameterHandle disjointParameter =
            disjoint.getParameterHandle(disjointInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle immediateFirst = immediate.getDimensionHandle(firstDimensionName);
         DimensionHandle immediateSecond = immediate.getDimensionHandle(secondDimensionName);
         DimensionHandle constrainedFirst = constrained.getDimensionHandle(firstDimensionName);
         DimensionHandle constrainedSecond = constrained.getDimensionHandle(secondDimensionName);
         DimensionHandle disjointFirst = disjoint.getDimensionHandle(firstDimensionName);
         DimensionHandle disjointSecond = disjoint.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(immediateInteraction)
               && ownerInteraction.equals(constrainedInteraction)
               && ownerInteraction.equals(disjointInteraction)
               && ownerParameter.equals(immediateParameter)
               && ownerParameter.equals(constrainedParameter)
               && ownerParameter.equals(disjointParameter)
               && ownerFirst.equals(immediateFirst) && ownerFirst.equals(constrainedFirst)
               && ownerFirst.equals(disjointFirst)
               && ownerSecond.equals(immediateSecond) && ownerSecond.equals(constrainedSecond)
               && ownerSecond.equals(disjointSecond),
            "adapter DDM FOM returned inconsistent interaction, parameter, or dimension handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);
         checkDimensions(immediate, immediateInteraction, immediateFirst, immediateSecond);
         checkDimensions(constrained, constrainedInteraction, constrainedFirst, constrainedSecond);
         checkDimensions(disjoint, disjointInteraction, disjointFirst, disjointSecond);

         owner.publishInteractionClass(ownerInteraction);
         published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 1, 5);
         immediateRegion = createRegion(immediate, immediateFirst, immediateSecond, 2, 6, 2, 6);
         constrainedRegion = createRegion(
            constrained, constrainedFirst, constrainedSecond, 2, 6, 2, 6);
         disjointRegion = createRegion(disjoint, disjointFirst, disjointSecond, 7, 9, 7, 9);
         sourceRegions = regionSet(owner, sourceRegion);
         immediateRegions = regionSet(immediate, immediateRegion);
         constrainedRegions = regionSet(constrained, constrainedRegion);
         disjointRegions = regionSet(disjoint, disjointRegion);
         immediate.setConveyRegionDesignatorSetsSwitch(true);
         constrained.setConveyRegionDesignatorSetsSwitch(true);
         disjoint.setConveyRegionDesignatorSetsSwitch(true);
         immediate.subscribeInteractionClassWithRegions(immediateInteraction, immediateRegions);
         immediateSubscribed = true;
         constrained.subscribeInteractionClassWithRegions(
            constrainedInteraction, constrainedRegions);
         constrainedSubscribed = true;
         disjoint.subscribeInteractionClassWithRegions(disjointInteraction, disjointRegions);
         disjointSubscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         check(timeFactory.getName().equals(constrained.getTimeFactory().getName()),
            "timestamped regional members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         constrained.enableTimeConstrained();
         timeConstrained = true;
         await(callbackModel, constrained,
            () -> constrainedRecorder.timeConstrainedEnabled, "timeConstrainedEnabled callback");
         owner.enableTimeRegulation(epsilon);
         regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabled,
            "timeRegulationEnabled callback");

         LogicalTime ownerInitial = owner.queryLogicalTime();
         LogicalTime constrainedInitial = constrained.queryLogicalTime();
         LogicalTime firstTime = addSteps(ownerInitial, epsilon, 3);
         LogicalTime firstConstrainedTime = addSteps(constrainedInitial, epsilon, 3);
         check(firstTime.equals(firstConstrainedTime),
            "publisher and constrained receiver derived different first advance targets");
         byte[] firstValue = new byte[] {0x01, 0x02, 0x03};
         byte[] firstTag = new byte[] {0x51, 0x31};
         MessageRetractionReturn firstRetraction = send(
            owner, ownerInteraction, ownerParameter, sourceRegions, firstValue, firstTag, firstTime);
         requireRetraction(firstRetraction, "first timestamped regional interaction");
         await(callbackModel, immediate,
            () -> immediateRecorder.interactions.size() == 1,
            "immediate timestamped regional interaction");
         assertInteraction(immediate, immediateRecorder.interactions.get(0),
            immediateInteraction, immediateParameter, producer, firstValue, firstTag,
            firstTime, OrderType.RECEIVE, firstRetraction.handle, sourceRegion);
         pump(disjoint, callbackModel);
         check(constrainedRecorder.interactions.isEmpty(),
            "constrained regional receiver delivered before its time grant");
         check(disjointRecorder.interactions.isEmpty(),
            "disjoint receiver received the first timestamped regional interaction");

         owner.retract(firstRetraction.handle);
         await(callbackModel, immediate,
            () -> immediateRecorder.retractions.size() == 1,
            "requestRetraction for delivered regional interaction");
         check(firstRetraction.handle.equals(immediateRecorder.retractions.get(0)),
            "regional interaction requestRetraction returned the wrong handle");
         check(constrainedRecorder.retractions.isEmpty(),
            "pending constrained regional interaction produced a retraction callback");
         constrained.timeAdvanceRequest(firstConstrainedTime);
         owner.timeAdvanceRequest(firstTime);
         await(callbackModel, owner, () -> ownerRecorder.grants.size() == 1,
            "first publisher timeAdvanceGrant");
         await(callbackModel, constrained, () -> constrainedRecorder.grants.size() == 1,
            "first constrained timeAdvanceGrant");
         check(firstTime.equals(ownerRecorder.grants.get(0))
               && firstConstrainedTime.equals(constrainedRecorder.grants.get(0)),
            "first timestamped regional grants reported the wrong logical times");
         pump(constrained, callbackModel);
         check(constrainedRecorder.interactions.isEmpty()
               && constrainedRecorder.retractions.isEmpty(),
            "retracted constrained regional interaction produced a stale callback at its grant");

         LogicalTime secondTime = addSteps(firstTime, epsilon, 3);
         LogicalTime secondConstrainedTime = addSteps(firstConstrainedTime, epsilon, 3);
         check(secondTime.equals(secondConstrainedTime),
            "publisher and constrained receiver derived different second advance targets");
         byte[] secondValue = new byte[] {0x21, 0x22};
         byte[] secondTag = new byte[] {0x51, 0x32};
         MessageRetractionReturn secondRetraction = send(
            owner, ownerInteraction, ownerParameter, sourceRegions,
            secondValue, secondTag, secondTime);
         requireRetraction(secondRetraction, "second timestamped regional interaction");
         await(callbackModel, immediate,
            () -> immediateRecorder.interactions.size() == 2,
            "second immediate timestamped regional interaction");
         assertInteraction(immediate, immediateRecorder.interactions.get(1),
            immediateInteraction, immediateParameter, producer, secondValue, secondTag,
            secondTime, OrderType.RECEIVE, secondRetraction.handle, sourceRegion);
         pump(disjoint, callbackModel);
         check(disjointRecorder.interactions.isEmpty(),
            "disjoint receiver received the second timestamped regional interaction");
         check(constrainedRecorder.interactions.isEmpty(),
            "constrained receiver received the second interaction before advancing");

         constrained.timeAdvanceRequest(secondConstrainedTime);
         owner.timeAdvanceRequest(secondTime);
         await(callbackModel, owner, () -> ownerRecorder.grants.size() == 2,
            "second publisher timeAdvanceGrant");
         await(callbackModel, constrained,
            () -> constrainedRecorder.grants.size() == 2
               && constrainedRecorder.interactions.size() == 1,
            "second constrained regional interaction and grant");
         check(secondTime.equals(ownerRecorder.grants.get(1))
               && secondConstrainedTime.equals(constrainedRecorder.grants.get(1)),
            "second timestamped regional grants reported the wrong logical times");
         assertInteraction(constrained, constrainedRecorder.interactions.get(0),
            constrainedInteraction, constrainedParameter, producer, secondValue, secondTag,
            secondTime, OrderType.TIMESTAMP, secondRetraction.handle, sourceRegion);
         check(constrainedRecorder.callbackOrder.equals(
               Arrays.asList("timeAdvanceGrant", "receiveInteraction", "timeAdvanceGrant")),
            "constrained reflection was not delivered before its matching grant");
         JavaTckSupport.expectFailure(() -> owner.retract(secondRetraction.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting the delivered timestamped regional interaction");

         constrained.disableTimeConstrained();
         timeConstrained = false;
         owner.disableTimeRegulation();
         regulating = false;
         disjoint.unsubscribeInteractionClassWithRegions(disjointInteraction, disjointRegions);
         disjointSubscribed = false;
         constrained.unsubscribeInteractionClassWithRegions(
            constrainedInteraction, constrainedRegions);
         constrainedSubscribed = false;
         immediate.unsubscribeInteractionClassWithRegions(immediateInteraction, immediateRegions);
         immediateSubscribed = false;
         owner.unpublishInteractionClass(ownerInteraction);
         published = false;
         owner.deleteRegion(sourceRegion);
         sourceRegion = null;
         immediate.deleteRegion(immediateRegion);
         immediateRegion = null;
         constrained.deleteRegion(constrainedRegion);
         constrainedRegion = null;
         disjoint.deleteRegion(disjointRegion);
         disjointRegion = null;
      } finally {
         if (timeConstrained && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (disjointSubscribed && disjointJoined) {
            try { disjoint.unsubscribeInteractionClassWithRegions(
               disjointInteraction, disjointRegions); } catch (Exception ignored) { }
         }
         if (constrainedSubscribed && constrainedJoined) {
            try { constrained.unsubscribeInteractionClassWithRegions(
               constrainedInteraction, constrainedRegions); } catch (Exception ignored) { }
         }
         if (immediateSubscribed && immediateJoined) {
            try { immediate.unsubscribeInteractionClassWithRegions(
               immediateInteraction, immediateRegions); } catch (Exception ignored) { }
         }
         if (published && ownerJoined) {
            try { owner.unpublishInteractionClass(ownerInteraction); } catch (Exception ignored) { }
         }
         deleteRegion(owner, sourceRegion, ownerJoined);
         deleteRegion(immediate, immediateRegion, immediateJoined);
         deleteRegion(constrained, constrainedRegion, constrainedJoined);
         deleteRegion(disjoint, disjointRegion, disjointJoined);
         if (disjointJoined) resign(disjoint);
         if (constrainedJoined) resign(constrained);
         if (immediateJoined) resign(immediate);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (disjointConnected) disconnect(disjoint);
         if (constrainedConnected) disconnect(constrained);
         if (immediateConnected) disconnect(immediate);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runDefaultRegion(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-default-region-interaction-" + UUID.randomUUID();
      boolean ownerConnected = false, receiverConnected = false, created = false;
      boolean ownerJoined = false, receiverJoined = false, published = false, subscribed = false;
      boolean regulating = false, timeConstrained = false;
      InteractionClassHandle ownerInteraction = null, receiverInteraction = null;
      RegionHandle receiverRegion = null;
      RegionHandleSet receiverRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-default-region-owner", federation);
         ownerJoined = true;
         receiver.joinFederationExecution("java-tck-default-region-member", federation);
         receiverJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(receiverInteraction)
               && ownerParameter.equals(receiverParameter)
               && ownerFirst.equals(receiverFirst) && ownerSecond.equals(receiverSecond),
            "adapter DDM FOM returned inconsistent interaction, parameter, or dimension handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);
         checkDimensions(receiver, receiverInteraction, receiverFirst, receiverSecond);

         owner.publishInteractionClass(ownerInteraction);
         published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6, 2, 6);
         receiverRegions = regionSet(receiver, receiverRegion);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "timestamped default-region members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         timeConstrained = true;
         await(callbackModel, receiver, () -> receiverRecorder.timeConstrainedEnabled,
            "timeConstrainedEnabled callback");
         owner.enableTimeRegulation(epsilon);
         regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabled,
            "timeRegulationEnabled callback");

         LogicalTime ownerInitial = owner.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime messageTime = addSteps(ownerInitial, epsilon, 6);
         LogicalTime receiverTarget = addSteps(receiverInitial, epsilon, 6);
         LogicalTime ownerTarget = addSteps(ownerInitial, epsilon, 2);
         check(messageTime.equals(receiverTarget),
            "publisher and constrained receiver derived different delivery times");
         byte[] value = new byte[] {(byte) 0xD3, 0x0F};
         byte[] tag = new byte[] {0x54, 0x53, 0x4F};
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, value.clone());
         MessageRetractionReturn retraction = owner.sendInteraction(
            ownerInteraction, parameters, tag.clone(), messageTime);
         requireRetraction(retraction, "timestamped default-region interaction");
         check(receiverRecorder.interactions.isEmpty(),
            "timestamped default-region interaction delivered before the recipient frontier");

         receiver.timeAdvanceRequest(receiverTarget);
         check(receiverRecorder.grants.isEmpty(),
            "timestamped default-region interaction granted before the publisher frontier");
         owner.timeAdvanceRequest(ownerTarget);
         await(callbackModel, owner, () -> ownerRecorder.grants.size() == 1,
            "publisher timeAdvanceGrant");
         await(callbackModel, receiver,
            () -> receiverRecorder.interactions.size() == 1 && receiverRecorder.grants.size() == 1,
            "timestamped default-region interaction delivery and grant");

         Interaction actual = receiverRecorder.interactions.get(0);
         check(actual.interaction.equals(receiverInteraction)
               && actual.parameters.size() == 1
               && Arrays.equals(value, actual.parameters.get(receiverParameter))
               && Arrays.equals(tag, actual.tag)
               && producer.equals(actual.producer)
               && messageTime.equals(actual.time)
               && actual.sentOrder == OrderType.TIMESTAMP
               && actual.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(actual.retraction)
               && actual.regions != null && actual.regions.isEmpty()
               && actual.transportation != null,
            "timestamped default-region interaction returned incorrect callback metadata");
         check(!receiver.getTransportationTypeName(actual.transportation).isEmpty()
               && actual.time.toString() != null && !actual.time.toString().isEmpty(),
            "timestamped default-region interaction returned unresolvable transport or time metadata");
         check(ownerRecorder.grants.size() == 1
               && ownerTarget.equals(ownerRecorder.grants.get(0))
               && receiverRecorder.grants.size() == 1
               && receiverTarget.equals(receiverRecorder.grants.get(0)),
            "timestamped default-region interaction returned the wrong grant time");
         check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "timestamped default-region interaction was not delivered before its grant");
         LogicalTime queriedTime = receiver.queryLogicalTime();
         check(receiverTarget.equals(queriedTime),
            "timestamped default-region interaction logical-time query returned the wrong value");
         JavaTckSupport.expectFailure(() -> owner.retract(retraction.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting the delivered timestamped default-region interaction");

         receiver.disableTimeConstrained();
         timeConstrained = false;
         owner.disableTimeRegulation();
         regulating = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = false;
         owner.unpublishInteractionClass(ownerInteraction);
         published = false;
         receiver.deleteRegion(receiverRegion);
         receiverRegion = null;
      } finally {
         if (timeConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeInteractionClassWithRegions(
               receiverInteraction, receiverRegions); } catch (Exception ignored) { }
         }
         if (published && ownerJoined) {
            try { owner.unpublishInteractionClass(ownerInteraction); } catch (Exception ignored) { }
         }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         if (receiverJoined) resign(receiver);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runDefaultRegionMixedFanout(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      RTIambassador immediate = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      Recorder immediateRecorder = new Recorder();
      String federation = "java-tck-timestamped-default-region-mixed-fanout-" + UUID.randomUUID();
      boolean ownerConnected = false, constrainedConnected = false, immediateConnected = false;
      boolean created = false, ownerJoined = false, constrainedJoined = false, immediateJoined = false;
      boolean published = false, constrainedSubscribed = false, immediateSubscribed = false;
      boolean regulating = false, timeConstrained = false;
      InteractionClassHandle ownerInteraction = null, constrainedInteraction = null;
      InteractionClassHandle immediateInteraction = null;
      RegionHandle constrainedRegion = null, immediateRegion = null;
      RegionHandleSet constrainedRegions = null, immediateRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;
         immediate.connect(immediateRecorder.proxy(), callbackModel); immediateConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-default-region-mixed-owner", federation); ownerJoined = true;
         constrained.joinFederationExecution("java-tck-default-region-mixed-constrained", federation);
         constrainedJoined = true;
         immediate.joinFederationExecution("java-tck-default-region-mixed-immediate", federation);
         immediateJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         constrainedInteraction = constrained.getInteractionClassHandle(interactionName);
         immediateInteraction = immediate.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle constrainedParameter =
            constrained.getParameterHandle(constrainedInteraction, parameterName);
         ParameterHandle immediateParameter =
            immediate.getParameterHandle(immediateInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle constrainedFirst = constrained.getDimensionHandle(firstDimensionName);
         DimensionHandle constrainedSecond = constrained.getDimensionHandle(secondDimensionName);
         DimensionHandle immediateFirst = immediate.getDimensionHandle(firstDimensionName);
         DimensionHandle immediateSecond = immediate.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(constrainedInteraction)
               && ownerInteraction.equals(immediateInteraction)
               && ownerParameter.equals(constrainedParameter)
               && ownerParameter.equals(immediateParameter)
               && ownerFirst.equals(constrainedFirst) && ownerFirst.equals(immediateFirst)
               && ownerSecond.equals(constrainedSecond) && ownerSecond.equals(immediateSecond),
            "adapter DDM FOM returned inconsistent mixed-fanout handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);
         checkDimensions(constrained, constrainedInteraction, constrainedFirst, constrainedSecond);
         checkDimensions(immediate, immediateInteraction, immediateFirst, immediateSecond);

         owner.publishInteractionClass(ownerInteraction); published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         constrainedRegion = createRegion(constrained, constrainedFirst, constrainedSecond, 2, 6, 2, 6);
         immediateRegion = createRegion(immediate, immediateFirst, immediateSecond, 2, 6, 2, 6);
         constrainedRegions = regionSet(constrained, constrainedRegion);
         immediateRegions = regionSet(immediate, immediateRegion);
         constrained.setConveyRegionDesignatorSetsSwitch(true);
         immediate.setConveyRegionDesignatorSetsSwitch(true);
         constrained.subscribeInteractionClassWithRegions(constrainedInteraction, constrainedRegions);
         constrainedSubscribed = true;
         immediate.subscribeInteractionClassWithRegions(immediateInteraction, immediateRegions);
         immediateSubscribed = true;

         LogicalTimeFactory<?, ?> ownerFactory = owner.getTimeFactory();
         check(ownerFactory.getName().equals(constrained.getTimeFactory().getName())
               && ownerFactory.getName().equals(immediate.getTimeFactory().getName()),
            "timestamped default-region mixed-fanout members selected different time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) ownerFactory.makeEpsilon();
         LogicalTimeInterval lookahead = epsilon;
         for (int i = 1; i < 5; ++i) {
            lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         }
         constrained.enableTimeConstrained(); timeConstrained = true;
         await(callbackModel, constrained, () -> constrainedRecorder.timeConstrainedEnabled,
            "mixed-fanout timeConstrainedEnabled callback");
         owner.enableTimeRegulation(lookahead); regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabled,
            "mixed-fanout timeRegulationEnabled callback");

         LogicalTime ownerInitial = owner.queryLogicalTime();
         LogicalTime constrainedInitial = constrained.queryLogicalTime();
         LogicalTime messageTime = addSteps(ownerInitial, epsilon, 6);
         LogicalTime constrainedTarget = addSteps(constrainedInitial, epsilon, 6);
         LogicalTime publisherTarget = addSteps(ownerInitial, epsilon, 2);
         check(messageTime.equals(constrainedTarget),
            "mixed-fanout publisher and constrained recipient derived different message times");
         byte[] value = new byte[] {0x44, 0x45, 0x46, 0x49};
         byte[] tag = new byte[] {0x44, 0x45, 0x46, 0x2D, 0x54, 0x53, 0x4F};
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, value.clone());
         constrainedRecorder.callbackOrder.clear();
         immediateRecorder.callbackOrder.clear();
         MessageRetractionReturn retraction = owner.sendInteraction(
            ownerInteraction, parameters, tag.clone(), messageTime);
         requireRetraction(retraction, "timestamped default-region mixed-fanout interaction");
         check(constrainedRecorder.interactions.isEmpty(),
            "mixed fanout delivered the constrained copy before its frontier");
         await(callbackModel, immediate, () -> immediateRecorder.interactions.size() == 1,
            "immediate default-region mixed-fanout delivery");
         check(immediateRecorder.interactions.size() == 1,
            "mixed fanout delivered duplicate immediate callbacks");
         Interaction immediateEvent = immediateRecorder.interactions.get(0);
         check(immediateEvent.interaction.equals(immediateInteraction)
               && immediateEvent.parameters.size() == 1
               && Arrays.equals(value, immediateEvent.parameters.get(immediateParameter))
               && Arrays.equals(tag, immediateEvent.tag)
               && producer.equals(immediateEvent.producer)
               && messageTime.equals(immediateEvent.time)
               && immediateEvent.sentOrder == OrderType.TIMESTAMP
               && immediateEvent.receivedOrder == OrderType.RECEIVE
               && retraction.handle.equals(immediateEvent.retraction)
               && immediateEvent.regions != null && immediateEvent.regions.isEmpty()
               && immediateEvent.transportation != null,
            "mixed fanout returned incorrect immediate interaction metadata");
         check(!immediate.getTransportationTypeName(immediateEvent.transportation).isEmpty(),
            "mixed fanout returned unresolved immediate transportation");
         check(immediateRecorder.callbackOrder.equals(Arrays.asList("receiveInteraction")),
            "mixed fanout returned the wrong immediate callback order");

         constrained.timeAdvanceRequest(constrainedTarget);
         check(constrainedRecorder.grants.isEmpty(),
            "mixed fanout granted before the publisher frontier");
         owner.retract(retraction.handle);
         check(constrainedRecorder.interactions.isEmpty()
               && constrainedRecorder.callbackOrder.isEmpty(),
            "mixed fanout leaked the retracted constrained callback");
         await(callbackModel, immediate, () -> immediateRecorder.retractions.size() == 1,
            "immediate mixed-fanout requestRetraction callback");
         check(immediateRecorder.retractions.size() == 1
               && retraction.handle.equals(immediateRecorder.retractions.get(0))
               && immediateRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "requestRetraction")),
            "mixed fanout returned the wrong immediate retraction callback");

         owner.timeAdvanceRequest(publisherTarget);
         long deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline
               && (ownerRecorder.grants.size() != 1 || constrainedRecorder.grants.size() != 1)) {
            pump(owner, callbackModel);
            pump(constrained, callbackModel);
            pump(immediate, callbackModel);
            Thread.sleep(1L);
         }
         check(ownerRecorder.grants.size() == 1 && constrainedRecorder.grants.size() == 1,
            "mixed-fanout publisher or constrained grant timed out");
         check(publisherTarget.equals(ownerRecorder.grants.get(0))
               && messageTime.equals(constrainedRecorder.grants.get(0))
               && constrainedRecorder.interactions.isEmpty()
               && constrainedRecorder.retractions.isEmpty()
               && constrainedRecorder.callbackOrder.equals(Arrays.asList("timeAdvanceGrant")),
            "mixed fanout returned an incorrect grant or leaked a retracted callback");
         JavaTckSupport.expectFailure(() -> owner.retract(retraction.handle),
            "MessageCanNoLongerBeRetracted", "terminal mixed-fanout retraction");

         constrained.disableTimeConstrained(); timeConstrained = false;
         owner.disableTimeRegulation(); regulating = false;
         constrained.unsubscribeInteractionClassWithRegions(constrainedInteraction, constrainedRegions);
         constrainedSubscribed = false;
         immediate.unsubscribeInteractionClassWithRegions(immediateInteraction, immediateRegions);
         immediateSubscribed = false;
         owner.unpublishInteractionClass(ownerInteraction); published = false;
         constrained.deleteRegion(constrainedRegion); constrainedRegion = null;
         immediate.deleteRegion(immediateRegion); immediateRegion = null;
      } finally {
         if (timeConstrained && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (constrainedSubscribed && constrainedJoined) {
            try { constrained.unsubscribeInteractionClassWithRegions(
               constrainedInteraction, constrainedRegions); } catch (Exception ignored) { }
         }
         if (immediateSubscribed && immediateJoined) {
            try { immediate.unsubscribeInteractionClassWithRegions(
               immediateInteraction, immediateRegions); } catch (Exception ignored) { }
         }
         if (published && ownerJoined) {
            try { owner.unpublishInteractionClass(ownerInteraction); } catch (Exception ignored) { }
         }
         deleteRegion(constrained, constrainedRegion, constrainedJoined);
         deleteRegion(immediate, immediateRegion, immediateJoined);
         if (immediateJoined) resign(immediate);
         if (constrainedJoined) resign(constrained);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (immediateConnected) disconnect(immediate);
         if (constrainedConnected) disconnect(constrained);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runDefaultRegionAlternateAdvances(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      runAlternateAdvances(factory, fom, timeImplementation, interactionName, parameterName,
         firstDimensionName, secondDimensionName, callbackModel, false);
   }

   static void runDefaultRegionSourceResignation(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      Recorder clockRecorder = new Recorder();
      String federation = "java-tck-default-region-source-resignation-" + UUID.randomUUID();
      boolean ownerConnected = false, firstConnected = false;
      boolean secondConnected = false, clockConnected = false, created = false;
      boolean ownerJoined = false, firstJoined = false, secondJoined = false, clockJoined = false;
      boolean firstSubscribed = false, secondSubscribed = false;
      boolean firstConstrained = false, secondConstrained = false;
      boolean ownerRegulating = false, clockRegulating = false;
      InteractionClassHandle ownerInteraction = null, firstInteraction = null;
      InteractionClassHandle secondInteraction = null;
      RegionHandle firstRegion = null, secondRegion = null;
      RegionHandleSet firstRegions = null, secondRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         first.connect(firstRecorder.proxy(), callbackModel); firstConnected = true;
         second.connect(secondRecorder.proxy(), callbackModel); secondConnected = true;
         clock.connect(clockRecorder.proxy(), callbackModel); clockConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-default-region-resigning-owner", federation); ownerJoined = true;
         first.joinFederationExecution("java-tck-default-region-resigning-first", federation);
         firstJoined = true;
         second.joinFederationExecution("java-tck-default-region-resigning-second", federation);
         secondJoined = true;
         clock.joinFederationExecution("java-tck-default-region-resigning-clock", federation);
         clockJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         firstInteraction = first.getInteractionClassHandle(interactionName);
         secondInteraction = second.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle firstParameter = first.getParameterHandle(firstInteraction, parameterName);
         ParameterHandle secondParameter = second.getParameterHandle(secondInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle firstDimension = first.getDimensionHandle(firstDimensionName);
         DimensionHandle firstSecondDimension = first.getDimensionHandle(secondDimensionName);
         DimensionHandle secondDimension = second.getDimensionHandle(firstDimensionName);
         DimensionHandle secondSecondDimension = second.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(firstInteraction) && ownerInteraction.equals(secondInteraction)
               && ownerParameter.equals(firstParameter) && ownerParameter.equals(secondParameter)
               && ownerFirst.equals(firstDimension) && ownerFirst.equals(secondDimension)
               && ownerSecond.equals(firstSecondDimension) && ownerSecond.equals(secondSecondDimension),
            "adapter DDM FOM returned inconsistent source-resignation handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);
         checkDimensions(first, firstInteraction, firstDimension, firstSecondDimension);
         checkDimensions(second, secondInteraction, secondDimension, secondSecondDimension);

         owner.publishInteractionClass(ownerInteraction);
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         firstRegion = createRegion(first, firstDimension, firstSecondDimension, 2, 6, 2, 6);
         secondRegion = createRegion(second, secondDimension, secondSecondDimension, 2, 6, 2, 6);
         firstRegions = regionSet(first, firstRegion);
         secondRegions = regionSet(second, secondRegion);
         first.setConveyRegionDesignatorSetsSwitch(true);
         second.setConveyRegionDesignatorSetsSwitch(true);
         first.subscribeInteractionClassWithRegions(firstInteraction, firstRegions);
         firstSubscribed = true;
         second.subscribeInteractionClassWithRegions(secondInteraction, secondRegions);
         secondSubscribed = true;

         LogicalTimeFactory<?, ?> ownerFactory = owner.getTimeFactory();
         String factoryName = ownerFactory.getName();
         check(factoryName.equals(first.getTimeFactory().getName())
               && factoryName.equals(second.getTimeFactory().getName())
               && factoryName.equals(clock.getTimeFactory().getName()),
            "source-resignation members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) ownerFactory.makeEpsilon();
         LogicalTimeInterval lookahead = epsilon;
         for (int i = 1; i < 5; ++i) lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         LogicalTimeInterval clockEpsilon =
            (LogicalTimeInterval) clock.getTimeFactory().makeEpsilon();
         LogicalTimeInterval clockLookahead = clockEpsilon;
         for (int i = 1; i < 5; ++i) {
            clockLookahead = (LogicalTimeInterval) clockLookahead.add(clockEpsilon);
         }

         first.enableTimeConstrained(); firstConstrained = true;
         await(callbackModel, first, () -> firstRecorder.timeConstrainedEnabled,
            "source-resignation first timeConstrainedEnabled callback");
         second.enableTimeConstrained(); secondConstrained = true;
         await(callbackModel, second, () -> secondRecorder.timeConstrainedEnabled,
            "source-resignation second timeConstrainedEnabled callback");
         owner.enableTimeRegulation(lookahead); ownerRegulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabled,
            "source-resignation producer timeRegulationEnabled callback");
         clock.enableTimeRegulation(clockLookahead); clockRegulating = true;
         await(callbackModel, clock, () -> clockRecorder.timeRegulationEnabled,
            "source-resignation clock timeRegulationEnabled callback");

         LogicalTime initial = owner.queryLogicalTime();
         LogicalTime firstInitial = first.queryLogicalTime();
         LogicalTime secondInitial = second.queryLogicalTime();
         LogicalTime clockInitial = clock.queryLogicalTime();
         check(initial.equals(firstInitial) && initial.equals(secondInitial)
               && initial.equals(clockInitial),
            "source-resignation members began at different logical times");
         LogicalTime messageTime = addSteps(initial, epsilon, 7);
         LogicalTime firstTarget = addSteps(firstInitial, epsilon, 7);
         LogicalTime secondTarget = addSteps(secondInitial, epsilon, 10);
         LogicalTime clockTarget = addSteps(clockInitial,
            clockEpsilon, 2);
         byte[] value = new byte[] {0x44, 0x45, 0x46, 0x2D, 0x46};
         byte[] tag = new byte[] {0x44, 0x45, 0x46, 0x2D, 0x52, 0x53};
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, value.clone());
         MessageRetractionReturn retraction = owner.sendInteraction(
            ownerInteraction, parameters, tag.clone(), messageTime);
         requireRetraction(retraction, "timestamped default-region source-resignation interaction");
         check(firstRecorder.interactions.isEmpty() && secondRecorder.interactions.isEmpty(),
            "source-resignation interaction arrived before either recipient frontier");

         first.timeAdvanceRequest(firstTarget);
         second.nextMessageRequest(secondTarget);
         check(firstRecorder.grants.isEmpty() && secondRecorder.grants.isEmpty(),
            "source-resignation recipients granted before the producer frontier");
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         check(firstRecorder.interactions.isEmpty() && secondRecorder.interactions.isEmpty()
               && firstRecorder.grants.isEmpty() && secondRecorder.grants.isEmpty(),
            "source resignation released queued interactions too early");

         clock.timeAdvanceRequest(clockTarget);
         long deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline) {
            pump(clock, callbackModel);
            pump(first, callbackModel);
            pump(second, callbackModel);
            if (clockRecorder.grants.size() == 1 && firstRecorder.interactions.size() == 1
                  && firstRecorder.grants.size() == 1 && secondRecorder.interactions.size() == 1
                  && secondRecorder.grants.size() == 1) break;
            Thread.sleep(1L);
         }
         check(clockRecorder.grants.size() == 1 && firstRecorder.interactions.size() == 1
               && firstRecorder.grants.size() == 1 && secondRecorder.interactions.size() == 1
               && secondRecorder.grants.size() == 1,
            "source-resignation clock frontier or queued recipient delivery timed out");

         for (int i = 0; i < 2; ++i) {
            RTIambassador receiver = i == 0 ? first : second;
            Recorder recorder = i == 0 ? firstRecorder : secondRecorder;
            InteractionClassHandle expectedInteraction = i == 0 ? firstInteraction : secondInteraction;
            ParameterHandle expectedParameter = receiver.getParameterHandle(
               expectedInteraction, parameterName);
            Interaction actual = recorder.interactions.get(0);
            check(actual.interaction.equals(expectedInteraction)
                  && actual.parameters.size() == 1
                  && Arrays.equals(value, actual.parameters.get(expectedParameter))
                  && Arrays.equals(tag, actual.tag)
                  && producer.equals(actual.producer)
                  && messageTime.equals(actual.time)
                  && actual.sentOrder == OrderType.TIMESTAMP
                  && actual.receivedOrder == OrderType.TIMESTAMP
                  && retraction.handle.equals(actual.retraction)
                  && actual.regions != null && actual.regions.isEmpty()
                  && actual.transportation != null,
               "source resignation did not preserve queued interaction callback metadata");
            check(!receiver.getTransportationTypeName(actual.transportation).isEmpty()
                  && recorder.callbackOrder.equals(
                     Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
               "source resignation returned invalid transport or delivery/grant order");
         }
         check(clockTarget.equals(clockRecorder.grants.get(0))
               && firstTarget.equals(firstRecorder.grants.get(0))
               && messageTime.equals(secondRecorder.grants.get(0)),
            "source resignation returned incorrect clock or recipient grant times");
         JavaTckSupport.expectFailure(() -> owner.retract(retraction.handle),
            "FederateNotExecutionMember", "retracting after interaction source resignation");

         first.disableTimeConstrained(); firstConstrained = false;
         second.disableTimeConstrained(); secondConstrained = false;
         clock.disableTimeRegulation(); clockRegulating = false;
         first.unsubscribeInteractionClassWithRegions(firstInteraction, firstRegions);
         firstSubscribed = false;
         second.unsubscribeInteractionClassWithRegions(secondInteraction, secondRegions);
         secondSubscribed = false;
         first.deleteRegion(firstRegion); firstRegion = null;
         second.deleteRegion(secondRegion); secondRegion = null;
      } finally {
         if (firstConstrained && firstJoined) {
            try { first.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (secondConstrained && secondJoined) {
            try { second.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (ownerRegulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (clockRegulating && clockJoined) {
            try { clock.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (firstSubscribed && firstJoined) {
            try { first.unsubscribeInteractionClassWithRegions(firstInteraction, firstRegions); }
            catch (Exception ignored) { }
         }
         if (secondSubscribed && secondJoined) {
            try { second.unsubscribeInteractionClassWithRegions(secondInteraction, secondRegions); }
            catch (Exception ignored) { }
         }
         deleteRegion(first, firstRegion, firstJoined);
         deleteRegion(second, secondRegion, secondJoined);
         if (secondJoined) resign(second);
         if (firstJoined) resign(first);
         if (clockJoined) resign(clock);
         if (ownerJoined) resign(owner);
         if (created) try { first.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (clockConnected) disconnect(clock);
         if (secondConnected) disconnect(second);
         if (firstConnected) disconnect(first);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runRegionalSourceResignation(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder(), receiverRecorder = new Recorder();
      Recorder clockRecorder = new Recorder();
      String federation = "java-tck-regional-source-resignation-" + UUID.randomUUID();
      boolean ownerConnected = false, receiverConnected = false, clockConnected = false;
      boolean created = false, ownerJoined = false, receiverJoined = false, clockJoined = false;
      boolean subscribed = false, constrained = false, ownerRegulating = false, clockRegulating = false;
      InteractionClassHandle ownerInteraction = null, receiverInteraction = null;
      RegionHandle sourceRegion = null, receiverRegion = null;
      RegionHandleSet sourceRegions = null, receiverRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel); receiverConnected = true;
         clock.connect(clockRecorder.proxy(), callbackModel); clockConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-regional-resigning-owner", federation); ownerJoined = true;
         receiver.joinFederationExecution("java-tck-regional-resigning-member", federation);
         receiverJoined = true;
         clock.joinFederationExecution("java-tck-regional-resigning-clock", federation);
         clockJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(receiverInteraction) && ownerParameter.equals(receiverParameter)
               && ownerFirst.equals(receiverFirst) && ownerSecond.equals(receiverSecond),
            "adapter DDM FOM returned inconsistent source-resignation handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);
         checkDimensions(receiver, receiverInteraction, receiverFirst, receiverSecond);
         owner.publishInteractionClass(ownerInteraction);
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6, 2, 6);
         sourceRegions = regionSet(owner, sourceRegion);
         receiverRegions = regionSet(receiver, receiverRegion);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         check(timeFactory.getName().equals(receiver.getTimeFactory().getName())
               && timeFactory.getName().equals(clock.getTimeFactory().getName()),
            "source-resignation members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval clockEpsilon =
            (LogicalTimeInterval) clock.getTimeFactory().makeEpsilon();
         LogicalTimeInterval lookahead = intervalSteps(epsilon, 5);
         LogicalTimeInterval clockLookahead = intervalSteps(clockEpsilon, 5);
         receiver.enableTimeConstrained(); constrained = true;
         await(callbackModel, receiver, () -> receiverRecorder.timeConstrainedEnabled,
            "source-resignation Time Constrained callback");
         owner.enableTimeRegulation(lookahead); ownerRegulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabled,
            "source-resignation producer Time Regulation callback");
         clock.enableTimeRegulation(clockLookahead); clockRegulating = true;
         await(callbackModel, clock, () -> clockRecorder.timeRegulationEnabled,
            "source-resignation clock Time Regulation callback");

         LogicalTime initial = owner.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime clockInitial = clock.queryLogicalTime();
         check(initial.equals(receiverInitial) && initial.equals(clockInitial),
            "source-resignation members began at different logical times");
         LogicalTime messageTime = addSteps(initial, epsilon, 6);
         LogicalTime receiverTarget = addSteps(receiverInitial,
            (LogicalTimeInterval) receiver.getTimeFactory().makeEpsilon(), 6);
         LogicalTime clockTarget = addSteps(clockInitial, clockEpsilon, 2);
         byte[] value = new byte[] {0x52, 0x45, 0x47, 0x53};
         byte[] tag = new byte[] {0x52, 0x45, 0x53, 0x49, 0x47};
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, value.clone());
         MessageRetractionReturn retraction = owner.sendInteractionWithRegions(
            ownerInteraction, parameters, sourceRegions, tag.clone(), messageTime);
         requireRetraction(retraction, "timestamped regional source-resignation interaction");
         check(receiverRecorder.interactions.isEmpty() && receiverRecorder.retractions.isEmpty(),
            "regional interaction arrived before the recipient frontier");
         receiver.timeAdvanceRequest(receiverTarget);
         check(receiverRecorder.grants.isEmpty(), "receiver granted before the independent clock");
         owner.resignFederationExecution(ResignAction.NO_ACTION); ownerJoined = false;
         check(receiverRecorder.interactions.isEmpty() && receiverRecorder.grants.isEmpty(),
            "producer resignation released the queued regional interaction too early");

         clock.timeAdvanceRequest(clockTarget);
         long deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline
               && (clockRecorder.grants.size() != 1 || receiverRecorder.interactions.size() != 1
                  || receiverRecorder.grants.size() != 1)) {
            pump(clock, callbackModel); pump(receiver, callbackModel); Thread.sleep(1L);
         }
         check(clockRecorder.grants.size() == 1 && receiverRecorder.interactions.size() == 1
               && receiverRecorder.grants.size() == 1,
            "source-resignation clock frontier or recipient delivery timed out");
         Interaction actual = receiverRecorder.interactions.get(0);
         check(actual.interaction.equals(receiverInteraction) && actual.parameters.size() == 1
               && Arrays.equals(value, actual.parameters.get(receiverParameter))
               && Arrays.equals(tag, actual.tag) && producer.equals(actual.producer)
               && messageTime.equals(actual.time) && actual.sentOrder == OrderType.TIMESTAMP
               && actual.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(actual.retraction)
               && actual.regions != null && actual.regions.size() == 1
               && actual.regions.contains(sourceRegion) && actual.transportation != null,
            "source resignation lost timestamped callback metadata or source-region designator");
         check(!receiver.getTransportationTypeName(actual.transportation).isEmpty()
               && clockTarget.equals(clockRecorder.grants.get(0))
               && receiverTarget.equals(receiverRecorder.grants.get(0))
               && receiverRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "source resignation returned invalid transport, grant, or callback ordering");
         JavaTckSupport.expectFailure(() -> owner.retract(retraction.handle),
            "FederateNotExecutionMember", "retracting after regional source resignation");

         receiver.disableTimeConstrained(); constrained = false;
         clock.disableTimeRegulation(); clockRegulating = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = false;
         receiver.deleteRegion(receiverRegion); receiverRegion = null;
          sourceRegion = null; // Its owner has resigned; the RTI retires its region.
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (ownerRegulating && ownerJoined) try { owner.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (clockRegulating && clockJoined) try { clock.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined) try {
            receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         } catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         deleteRegion(owner, sourceRegion, ownerJoined);
         if (receiverJoined) resign(receiver);
         if (clockJoined) resign(clock);
         if (ownerJoined) resign(owner);
         if (created) try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (clockConnected) disconnect(clock);
         if (receiverConnected) disconnect(receiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runRegionalRegulationReenable(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder(), receiverRecorder = new Recorder();
      String federation = "java-tck-regional-regulation-reenable-" + UUID.randomUUID();
      boolean ownerConnected = false, receiverConnected = false, created = false;
      boolean ownerJoined = false, receiverJoined = false, published = false, subscribed = false;
      boolean regulating = false, constrained = false;
      InteractionClassHandle ownerInteraction = null, receiverInteraction = null;
      RegionHandle sourceRegion = null, receiverRegion = null;
      RegionHandleSet sourceRegions = null, receiverRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel); receiverConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-regional-regulation-owner", federation); ownerJoined = true;
         receiver.joinFederationExecution("java-tck-regional-regulation-member", federation);
         receiverJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(receiverInteraction) && ownerParameter.equals(receiverParameter)
               && ownerFirst.equals(receiverFirst) && ownerSecond.equals(receiverSecond),
            "adapter DDM FOM returned inconsistent regulation-reenable handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);
         checkDimensions(receiver, receiverInteraction, receiverFirst, receiverSecond);

         owner.publishInteractionClass(ownerInteraction); published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6, 2, 6);
         sourceRegions = regionSet(owner, sourceRegion);
         receiverRegions = regionSet(receiver, receiverRegion);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "regional regulation-reenable members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval initialLookahead = intervalSteps(epsilon, 5);
         LogicalTimeInterval changedLookahead = intervalSteps(epsilon, 3);
         receiver.enableTimeConstrained(); constrained = true;
         await(callbackModel, receiver, () -> receiverRecorder.timeConstrainedEnabled,
            "regional regulation-reenable Time Constrained callback");
         owner.enableTimeRegulation(initialLookahead); regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabledCount >= 1,
            "regional regulation-reenable initial Time Regulation callback");

         LogicalTime initial = owner.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         check(initial.equals(receiverInitial),
            "regional regulation-reenable members began at different logical times");
         LogicalTime messageTime = addSteps(initial, epsilon, 5);
         LogicalTime ownerTarget = addSteps(initial, epsilon, 2);
         byte[] value = new byte[] {(byte) 0xC7, 0x19};
         byte[] tag = new byte[] {0x52, 0x47, 0x43, 0x48};
         MessageRetractionReturn retraction = send(owner, ownerInteraction, ownerParameter,
            sourceRegions, value, tag, messageTime);
         requireRetraction(retraction, "timestamped regional regulation-reenable interaction");
         check(receiverRecorder.interactions.isEmpty(),
            "regional regulation-reenable interaction delivered before its grant");

         owner.setRangeBounds(sourceRegion, ownerFirst, new RangeBounds(7, 9));
         owner.setRangeBounds(sourceRegion, ownerSecond, new RangeBounds(7, 9));
         owner.commitRegionModifications(sourceRegions);
         owner.disableTimeRegulation(); regulating = false;
         owner.enableTimeRegulation(changedLookahead); regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabledCount >= 2,
            "regional regulation-reenable changed-lookahead callback");
         check(changedLookahead.equals(owner.queryLookahead()),
            "Query Lookahead did not return the changed regional interaction interval");
         pump(receiver, callbackModel);
         check(receiverRecorder.interactions.isEmpty(),
            "queued regional interaction delivered during Time Regulation re-enable");

         receiverRecorder.callbackOrder.clear();
         ownerRecorder.callbackOrder.clear();
         receiver.timeAdvanceRequest(messageTime);
         check(receiverRecorder.interactions.isEmpty(),
            "regional interaction delivered while the producer boundary was pending");
         owner.timeAdvanceRequest(ownerTarget);
         long deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline
               && (ownerRecorder.grants.size() != 1 || receiverRecorder.grants.size() != 1
                  || receiverRecorder.interactions.size() != 1)) {
            pump(owner, callbackModel); pump(receiver, callbackModel); Thread.sleep(1L);
         }
         check(ownerRecorder.grants.size() == 1 && receiverRecorder.grants.size() == 1
               && receiverRecorder.interactions.size() == 1
               && ownerRecorder.timeRegulationEnabledCount == 2,
            "regional regulation-reenabled interaction delivery or grants timed out");
         check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "regional regulation-reenabled interaction was delivered after its grant");
         Interaction actual = receiverRecorder.interactions.get(0);
         check(actual.interaction.equals(receiverInteraction) && actual.parameters.size() == 1
               && Arrays.equals(value, actual.parameters.get(receiverParameter))
               && Arrays.equals(tag, actual.tag) && producer.equals(actual.producer)
               && messageTime.equals(actual.time) && actual.sentOrder == OrderType.TIMESTAMP
               && actual.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(actual.retraction)
               && actual.regions != null && actual.regions.size() == 1
               && actual.regions.contains(sourceRegion) && actual.transportation != null,
            "regional regulation-reenabled callback lost payload or invocation-time region metadata");
         check(!receiver.getTransportationTypeName(actual.transportation).isEmpty()
               && ownerTarget.equals(ownerRecorder.grants.get(0))
               && messageTime.equals(receiverRecorder.grants.get(0)),
            "regional regulation-reenabled interaction returned unresolved transport or wrong grants");
         JavaTckSupport.expectFailure(() -> owner.retract(retraction.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting regional interaction after delivery following Time Regulation re-enable");

         receiver.disableTimeConstrained(); constrained = false;
         owner.disableTimeRegulation(); regulating = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = false;
         receiver.deleteRegion(receiverRegion); receiverRegion = null;
         owner.deleteRegion(sourceRegion); sourceRegion = null;
         owner.unpublishInteractionClass(ownerInteraction); published = false;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && ownerJoined) try { owner.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined) try {
            receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         } catch (Exception ignored) { }
         if (published && ownerJoined) try { owner.unpublishInteractionClass(ownerInteraction); }
            catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         deleteRegion(owner, sourceRegion, ownerJoined);
         if (receiverJoined) resign(receiver);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runRegionalNoOverlapRetraction(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder(), receiverRecorder = new Recorder();
      String federation = "java-tck-regional-no-overlap-retraction-" + UUID.randomUUID();
      boolean ownerConnected = false, receiverConnected = false, created = false;
      boolean ownerJoined = false, receiverJoined = false, published = false, subscribed = false;
      boolean regulating = false, constrained = false;
      InteractionClassHandle ownerInteraction = null, receiverInteraction = null;
      RegionHandle sourceRegion = null, receiverRegion = null;
      RegionHandleSet sourceRegions = null, receiverRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel); receiverConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         owner.joinFederationExecution("java-tck-regional-no-overlap-owner", federation);
         ownerJoined = true;
         receiver.joinFederationExecution("java-tck-regional-no-overlap-member", federation);
         receiverJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(receiverInteraction) && ownerParameter.equals(receiverParameter)
               && ownerFirst.equals(receiverFirst) && ownerSecond.equals(receiverSecond),
            "adapter DDM FOM returned inconsistent no-overlap handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);

         owner.publishInteractionClass(ownerInteraction); published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 7, 9, 7, 9);
         sourceRegions = regionSet(owner, sourceRegion);
         receiverRegions = regionSet(receiver, receiverRegion);
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "no-overlap members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         receiver.enableTimeConstrained(); constrained = true;
         await(callbackModel, receiver, () -> receiverRecorder.timeConstrainedEnabled,
            "no-overlap Time Constrained callback");
         owner.enableTimeRegulation(epsilon); regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabledCount >= 1,
            "no-overlap Time Regulation callback");

         LogicalTime initial = owner.queryLogicalTime();
         check(initial.equals(receiver.queryLogicalTime()),
            "no-overlap members began at different logical times");
         LogicalTime messageTime = addSteps(initial, epsilon, 6);
         LogicalTime ownerTarget = addSteps(initial, epsilon, 1);
         byte[] value = new byte[] {0x4E, 0x4F};
         byte[] tag = new byte[] {0x4F, 0x56, 0x45, 0x52};
         MessageRetractionReturn retracted = send(owner, ownerInteraction, ownerParameter,
            sourceRegions, value, tag, messageTime);
         requireRetraction(retracted, "timestamped regional no-overlap first send");
         check(receiverRecorder.interactions.isEmpty(),
            "non-overlapping subscriber received the first timestamped interaction");
         owner.retract(retracted.handle);
         JavaTckSupport.expectFailure(() -> owner.retract(retracted.handle),
            "MessageCanNoLongerBeRetracted", "second retraction of filtered regional interaction");

         MessageRetractionReturn expired = send(owner, ownerInteraction, ownerParameter,
            sourceRegions, value, tag, messageTime);
         requireRetraction(expired, "timestamped regional no-overlap second send");
         owner.timeAdvanceRequest(ownerTarget);
         await(callbackModel, owner, () -> !ownerRecorder.grants.isEmpty(),
            "no-overlap producer time advance grant");
         JavaTckSupport.expectFailure(() -> owner.retract(expired.handle),
            "MessageCanNoLongerBeRetracted", "retraction after producer advanced past message time");
         for (int pass = 0; pass < 8; ++pass) pump(receiver, callbackModel);
         check(receiverRecorder.interactions.isEmpty(),
            "non-overlapping subscriber received the later timestamped interaction");

         receiver.disableTimeConstrained(); constrained = false;
         owner.disableTimeRegulation(); regulating = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = false;
         receiver.deleteRegion(receiverRegion); receiverRegion = null;
         owner.deleteRegion(sourceRegion); sourceRegion = null;
         owner.unpublishInteractionClass(ownerInteraction); published = false;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && ownerJoined) try { owner.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined) try {
            receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         } catch (Exception ignored) { }
         if (published && ownerJoined) try { owner.unpublishInteractionClass(ownerInteraction); }
            catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         deleteRegion(owner, sourceRegion, ownerJoined);
         if (receiverJoined) resign(receiver);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runRegionalSubscriptionReplacement(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder(), receiverRecorder = new Recorder();
      String federation = "java-tck-regional-subscription-replacement-" + UUID.randomUUID();
      boolean ownerConnected = false, receiverConnected = false, created = false;
      boolean ownerJoined = false, receiverJoined = false, published = false, subscribed = false;
      boolean regulating = false, constrained = false;
      InteractionClassHandle ownerInteraction = null, receiverInteraction = null;
      RegionHandle sourceRegionA = null, sourceRegionB = null;
      RegionHandle receiverRegionA = null, receiverRegionB = null;
      RegionHandleSet sourceRegionsA = null, sourceRegionsB = null;
      RegionHandleSet receiverRegionsA = null, receiverRegionsB = null;
      RegionHandleSet activeSubscription = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel); receiverConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-regional-subscription-owner", federation); ownerJoined = true;
         receiver.joinFederationExecution("java-tck-regional-subscription-member", federation);
         receiverJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(receiverInteraction) && ownerParameter.equals(receiverParameter)
               && ownerFirst.equals(receiverFirst) && ownerSecond.equals(receiverSecond),
            "adapter DDM FOM returned inconsistent subscription-replacement handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);

         owner.publishInteractionClass(ownerInteraction); published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         sourceRegionA = createRegion(owner, ownerFirst, ownerSecond, 0, 1, 0, 1);
         sourceRegionB = createRegion(owner, ownerFirst, ownerSecond, 2, 3, 2, 3);
         receiverRegionA = createRegion(receiver, receiverFirst, receiverSecond, 0, 1, 0, 1);
         receiverRegionB = createRegion(receiver, receiverFirst, receiverSecond, 2, 3, 2, 3);
         sourceRegionsA = regionSet(owner, sourceRegionA);
         sourceRegionsB = regionSet(owner, sourceRegionB);
         receiverRegionsA = regionSet(receiver, receiverRegionA);
         receiverRegionsB = regionSet(receiver, receiverRegionB);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "subscription replacement did not enable region-designator callbacks");
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegionsA);
         subscribed = true; activeSubscription = receiverRegionsA;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "subscription-replacement members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         receiver.enableTimeConstrained(); constrained = true;
         await(callbackModel, receiver, () -> receiverRecorder.timeConstrainedEnabled,
            "subscription-replacement Time Constrained callback");
         owner.enableTimeRegulation(epsilon); regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabledCount >= 1,
            "subscription-replacement Time Regulation callback");
         receiverRecorder.callbackOrder.clear(); ownerRecorder.callbackOrder.clear();

         LogicalTime initial = owner.queryLogicalTime();
         check(initial.equals(receiver.queryLogicalTime()),
            "subscription-replacement members began at different logical times");
         LogicalTime firstTime = addSteps(initial, epsilon, 6);
         LogicalTime firstOwnerTarget = addSteps(initial, epsilon, 2);
         byte[] firstValue = new byte[] {0x49, 0x52, 0x31};
         byte[] secondValue = new byte[] {0x49, 0x52, 0x32};
         byte[] firstTag = new byte[] {0x53, 0x55, 0x42, 0x31};
         byte[] secondTag = new byte[] {0x53, 0x55, 0x42, 0x32};
         MessageRetractionReturn firstRetraction = send(owner, ownerInteraction, ownerParameter,
            sourceRegionsA, firstValue, firstTag, firstTime);
         requireRetraction(firstRetraction, "subscription-replacement first interaction");
         check(receiverRecorder.interactions.isEmpty(),
            "first regional interaction delivered before the receiver grant");

         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegionsA);
         subscribed = false; activeSubscription = null;
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegionsB);
         subscribed = true; activeSubscription = receiverRegionsB;
         receiver.timeAdvanceRequest(firstTime);
         owner.timeAdvanceRequest(firstOwnerTarget);
         long deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline
               && (ownerRecorder.grants.size() < 1 || receiverRecorder.grants.size() < 1)) {
            pump(owner, callbackModel); pump(receiver, callbackModel); Thread.sleep(1L);
         }
         check(ownerRecorder.grants.size() == 1 && receiverRecorder.grants.size() == 1
               && receiverRecorder.interactions.isEmpty() && receiverRecorder.retractions.isEmpty()
               && receiverRecorder.callbackOrder.equals(Arrays.asList("timeAdvanceGrant")),
            "subscription replacement retargeted or delivered the first queued passel");
         receiverRecorder.callbackOrder.clear(); ownerRecorder.callbackOrder.clear();

         LogicalTime secondTime = addSteps(initial, epsilon, 7);
         LogicalTime secondOwnerTarget = addSteps(initial, epsilon, 3);
         MessageRetractionReturn secondRetraction = send(owner, ownerInteraction, ownerParameter,
            sourceRegionsB, secondValue, secondTag, secondTime);
         requireRetraction(secondRetraction, "subscription-replacement second interaction");
         receiver.timeAdvanceRequest(secondTime);
         owner.timeAdvanceRequest(secondOwnerTarget);
         deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline
               && (ownerRecorder.grants.size() < 2 || receiverRecorder.grants.size() < 2
                  || receiverRecorder.interactions.size() < 1)) {
            pump(owner, callbackModel); pump(receiver, callbackModel); Thread.sleep(1L);
         }
         check(ownerRecorder.grants.size() == 2 && receiverRecorder.grants.size() == 2
               && receiverRecorder.interactions.size() == 1 && receiverRecorder.retractions.isEmpty(),
            "subscription replacement returned the wrong second-delivery or grant counts");
         Interaction actual = receiverRecorder.interactions.get(0);
         assertInteraction(receiver, actual, receiverInteraction, receiverParameter, producer,
            secondValue, secondTag, secondTime, OrderType.TIMESTAMP, secondRetraction.handle,
            sourceRegionB);
         check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "replacement interaction was delivered after the receiver grant");

         receiver.disableTimeConstrained(); constrained = false;
         owner.disableTimeRegulation(); regulating = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegionsB);
         subscribed = false; activeSubscription = null;
         owner.unpublishInteractionClass(ownerInteraction); published = false;
         receiver.deleteRegion(receiverRegionB); receiverRegionB = null;
         receiver.deleteRegion(receiverRegionA); receiverRegionA = null;
         owner.deleteRegion(sourceRegionB); sourceRegionB = null;
         owner.deleteRegion(sourceRegionA); sourceRegionA = null;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && ownerJoined) try { owner.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined && activeSubscription != null) try {
            receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, activeSubscription);
         } catch (Exception ignored) { }
         if (published && ownerJoined) try { owner.unpublishInteractionClass(ownerInteraction); }
            catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegionB, receiverJoined);
         deleteRegion(receiver, receiverRegionA, receiverJoined);
         deleteRegion(owner, sourceRegionB, ownerJoined);
         deleteRegion(owner, sourceRegionA, ownerJoined);
         if (receiverJoined) resign(receiver);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runRegionalTarNmr(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador tar = factory.getRtiAmbassador();
      RTIambassador nmr = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder(), tarRecorder = new Recorder();
      Recorder nmrRecorder = new Recorder();
      String federation = "java-tck-regional-tar-nmr-" + UUID.randomUUID();
      boolean ownerConnected = false, tarConnected = false, nmrConnected = false;
      boolean created = false, ownerJoined = false, tarJoined = false, nmrJoined = false;
      boolean published = false, tarSubscribed = false, nmrSubscribed = false;
      boolean ownerRegulating = false, tarConstrained = false, nmrConstrained = false;
      InteractionClassHandle ownerInteraction = null, tarInteraction = null, nmrInteraction = null;
      RegionHandle sourceRegion = null, tarRegion = null, nmrRegion = null;
      RegionHandleSet sourceRegions = null, tarRegions = null, nmrRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         tar.connect(tarRecorder.proxy(), callbackModel); tarConnected = true;
         nmr.connect(nmrRecorder.proxy(), callbackModel); nmrConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-regional-tar-nmr-owner", federation); ownerJoined = true;
         tar.joinFederationExecution("java-tck-regional-tar-member", federation);
         tarJoined = true;
         nmr.joinFederationExecution("java-tck-regional-nmr-member", federation);
         nmrJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         tarInteraction = tar.getInteractionClassHandle(interactionName);
         nmrInteraction = nmr.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle tarParameter = tar.getParameterHandle(tarInteraction, parameterName);
         ParameterHandle nmrParameter = nmr.getParameterHandle(nmrInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle tarFirst = tar.getDimensionHandle(firstDimensionName);
         DimensionHandle tarSecond = tar.getDimensionHandle(secondDimensionName);
         DimensionHandle nmrFirst = nmr.getDimensionHandle(firstDimensionName);
         DimensionHandle nmrSecond = nmr.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(tarInteraction) && ownerInteraction.equals(nmrInteraction)
               && ownerParameter.equals(tarParameter) && ownerParameter.equals(nmrParameter)
               && ownerFirst.equals(tarFirst) && ownerFirst.equals(nmrFirst)
               && ownerSecond.equals(tarSecond) && ownerSecond.equals(nmrSecond),
            "adapter DDM FOM returned inconsistent TAR/NMR interaction handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);

         owner.publishInteractionClass(ownerInteraction); published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 1, 5);
         tarRegion = createRegion(tar, tarFirst, tarSecond, 2, 6, 2, 6);
         nmrRegion = createRegion(nmr, nmrFirst, nmrSecond, 2, 6, 2, 6);
         sourceRegions = regionSet(owner, sourceRegion);
         tarRegions = regionSet(tar, tarRegion);
         nmrRegions = regionSet(nmr, nmrRegion);
         tar.setConveyRegionDesignatorSetsSwitch(true);
         nmr.setConveyRegionDesignatorSetsSwitch(true);
         tar.subscribeInteractionClassWithRegions(tarInteraction, tarRegions);
         tarSubscribed = true;
         nmr.subscribeInteractionClassWithRegions(nmrInteraction, nmrRegions);
         nmrSubscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         check(timeFactory.getName().equals(tar.getTimeFactory().getName())
               && timeFactory.getName().equals(nmr.getTimeFactory().getName()),
            "regional TAR/NMR members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalSteps(epsilon, 5);
         tar.enableTimeConstrained(); tarConstrained = true;
         await(callbackModel, tar, () -> tarRecorder.timeConstrainedEnabled,
            "regional TAR member Time Constrained callback");
         nmr.enableTimeConstrained(); nmrConstrained = true;
         await(callbackModel, nmr, () -> nmrRecorder.timeConstrainedEnabled,
            "regional NMR member Time Constrained callback");
         owner.enableTimeRegulation(lookahead); ownerRegulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabledCount >= 1,
            "regional TAR/NMR owner Time Regulation callback");

         LogicalTime initial = owner.queryLogicalTime();
         LogicalTime tarInitial = tar.queryLogicalTime();
         LogicalTime nmrInitial = nmr.queryLogicalTime();
         check(initial.equals(tarInitial) && initial.equals(nmrInitial),
            "regional TAR/NMR members began at different logical times");
         LogicalTime messageTime = addSteps(initial, epsilon, 7);
         LogicalTime tarTarget = addSteps(tarInitial,
            (LogicalTimeInterval) tar.getTimeFactory().makeEpsilon(), 7);
         LogicalTime nmrTarget = addSteps(nmrInitial,
            (LogicalTimeInterval) nmr.getTimeFactory().makeEpsilon(), 10);
         LogicalTime ownerTarget = addSteps(initial, epsilon, 2);
         byte[] value = new byte[] {0x54, 0x41, 0x52, 0x2D, 0x4E};
         byte[] tag = new byte[] {0x54, 0x41, 0x52, 0x2D, 0x4D};
         MessageRetractionReturn retraction = send(owner, ownerInteraction, ownerParameter,
            sourceRegions, value, tag, messageTime);
         requireRetraction(retraction, "timestamped regional TAR/NMR interaction");
         check(tarRecorder.interactions.isEmpty() && nmrRecorder.interactions.isEmpty(),
            "regional TAR/NMR interaction delivered before either request");

         tarRecorder.callbackOrder.clear(); nmrRecorder.callbackOrder.clear();
         ownerRecorder.callbackOrder.clear();
         tar.timeAdvanceRequest(tarTarget);
         nmr.nextMessageRequest(nmrTarget);
         check(tarRecorder.grants.isEmpty() && nmrRecorder.grants.isEmpty(),
            "regional TAR/NMR member granted before the publisher frontier");
         owner.timeAdvanceRequest(ownerTarget);
         long deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline
               && (ownerRecorder.grants.size() < 1 || tarRecorder.grants.size() < 1
                  || tarRecorder.interactions.size() < 1 || nmrRecorder.grants.size() < 1
                  || nmrRecorder.interactions.size() < 1)) {
            pump(owner, callbackModel); pump(tar, callbackModel); pump(nmr, callbackModel);
            Thread.sleep(1L);
         }
         check(ownerRecorder.grants.size() == 1 && tarRecorder.grants.size() == 1
               && tarRecorder.interactions.size() == 1 && nmrRecorder.grants.size() == 1
               && nmrRecorder.interactions.size() == 1,
            "regional TAR/NMR interactions or grants timed out or duplicated");

         Interaction tarActual = tarRecorder.interactions.get(0);
         Interaction nmrActual = nmrRecorder.interactions.get(0);
         assertInteraction(tar, tarActual, tarInteraction, tarParameter, producer,
            value, tag, messageTime, OrderType.TIMESTAMP, retraction.handle, sourceRegion);
         assertInteraction(nmr, nmrActual, nmrInteraction, nmrParameter, producer,
            value, tag, messageTime, OrderType.TIMESTAMP, retraction.handle, sourceRegion);
         check(tarTarget.equals(tarRecorder.grants.get(0))
               && messageTime.equals(nmrRecorder.grants.get(0))
               && tarRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "timeAdvanceGrant"))
               && nmrRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "regional TAR/NMR returned incorrect grant frontiers or callback ordering");
         check(tarTarget.equals(tar.queryLogicalTime())
               && messageTime.equals(nmr.queryLogicalTime()),
            "regional TAR/NMR query returned an incorrect logical time");

         tar.disableTimeConstrained(); tarConstrained = false;
         nmr.disableTimeConstrained(); nmrConstrained = false;
         owner.disableTimeRegulation(); ownerRegulating = false;
         tar.unsubscribeInteractionClassWithRegions(tarInteraction, tarRegions);
         tarSubscribed = false;
         nmr.unsubscribeInteractionClassWithRegions(nmrInteraction, nmrRegions);
         nmrSubscribed = false;
         owner.unpublishInteractionClass(ownerInteraction); published = false;
         tar.deleteRegion(tarRegion); tarRegion = null;
         nmr.deleteRegion(nmrRegion); nmrRegion = null;
         owner.deleteRegion(sourceRegion); sourceRegion = null;
      } finally {
         if (tarConstrained && tarJoined) try { tar.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (nmrConstrained && nmrJoined) try { nmr.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (ownerRegulating && ownerJoined) try { owner.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (tarSubscribed && tarJoined) try {
            tar.unsubscribeInteractionClassWithRegions(tarInteraction, tarRegions);
         } catch (Exception ignored) { }
         if (nmrSubscribed && nmrJoined) try {
            nmr.unsubscribeInteractionClassWithRegions(nmrInteraction, nmrRegions);
         } catch (Exception ignored) { }
         if (published && ownerJoined) try { owner.unpublishInteractionClass(ownerInteraction); }
            catch (Exception ignored) { }
         deleteRegion(tar, tarRegion, tarJoined);
         deleteRegion(nmr, nmrRegion, nmrJoined);
         deleteRegion(owner, sourceRegion, ownerJoined);
         if (nmrJoined) resign(nmr);
         if (tarJoined) resign(tar);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (nmrConnected) disconnect(nmr);
         if (tarConnected) disconnect(tar);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runCustomTransportationRegional(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String transportationName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      // Retain the complete standard regional-interaction oracle before the
      // focused assertion that the adapter-declared FOM transport survives TSO.
      run(factory, fom, timeImplementation, interactionName, parameterName,
         firstDimensionName, secondDimensionName, callbackModel);

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder(), receiverRecorder = new Recorder();
      String federation = "java-tck-custom-transport-regional-interaction-" + UUID.randomUUID();
      boolean ownerConnected = false, receiverConnected = false, created = false;
      boolean ownerJoined = false, receiverJoined = false, published = false, subscribed = false;
      boolean regulating = false, constrained = false;
      InteractionClassHandle ownerInteraction = null, receiverInteraction = null;
      RegionHandle sourceRegion = null, receiverRegion = null;
      RegionHandleSet sourceRegions = null, receiverRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel); receiverConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-custom-transport-regional-owner", federation); ownerJoined = true;
         receiver.joinFederationExecution("java-tck-custom-transport-regional-member", federation);
         receiverJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         TransportationTypeHandle ownerTransportation =
            owner.getTransportationTypeHandle(transportationName);
         TransportationTypeHandle receiverTransportation =
            receiver.getTransportationTypeHandle(transportationName);
         check(ownerInteraction.equals(receiverInteraction) && ownerParameter.equals(receiverParameter)
               && ownerFirst.equals(receiverFirst) && ownerSecond.equals(receiverSecond)
               && ownerTransportation != null && receiverTransportation != null
               && ownerTransportation.encodedLength() > 0 && receiverTransportation.encodedLength() > 0
               && transportationName.equals(owner.getTransportationTypeName(ownerTransportation))
               && transportationName.equals(receiver.getTransportationTypeName(receiverTransportation)),
            "adapter FOM interaction, parameter, dimensions, or custom transportation did not round-trip");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);
         checkDimensions(receiver, receiverInteraction, receiverFirst, receiverSecond);

         owner.publishInteractionClass(ownerInteraction); published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6, 2, 6);
         sourceRegions = regionSet(owner, sourceRegion);
         receiverRegions = regionSet(receiver, receiverRegion);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "custom-transport regional interaction did not enable region designator callbacks");
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "custom-transport regional members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         receiver.enableTimeConstrained(); constrained = true;
         await(callbackModel, receiver, () -> receiverRecorder.timeConstrainedEnabled,
            "custom-transport regional Time Constrained callback");
         owner.enableTimeRegulation(epsilon); regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabledCount >= 1,
            "custom-transport regional Time Regulation callback");

         LogicalTime initial = owner.queryLogicalTime();
         check(initial.equals(receiver.queryLogicalTime()),
            "custom-transport regional members began at different logical times");
         LogicalTime messageTime = addSteps(initial, epsilon, 3);
         LogicalTime receiverTarget = addSteps(initial,
            (LogicalTimeInterval) receiver.getTimeFactory().makeEpsilon(), 3);
         byte[] value = new byte[] {(byte) 0xC5, 0x52, 0x49};
         byte[] tag = new byte[] {0x54, 0x52, 0x49};
         MessageRetractionReturn retraction = send(owner, ownerInteraction, ownerParameter,
            sourceRegions, value, tag, messageTime);
         requireRetraction(retraction, "custom-transport timestamped regional interaction");
         check(receiverRecorder.interactions.isEmpty(),
            "custom-transport regional interaction delivered before its grant");
         receiver.timeAdvanceRequest(receiverTarget);
         owner.timeAdvanceRequest(messageTime);
         long deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline
               && (ownerRecorder.grants.size() != 1 || receiverRecorder.grants.size() != 1
                  || receiverRecorder.interactions.size() != 1)) {
            pump(owner, callbackModel); pump(receiver, callbackModel); Thread.sleep(1L);
         }
         check(ownerRecorder.grants.size() == 1 && receiverRecorder.grants.size() == 1
               && receiverRecorder.interactions.size() == 1,
            "custom-transport regional delivery or grants timed out or duplicated");
         Interaction actual = receiverRecorder.interactions.get(0);
         assertInteraction(receiver, actual, receiverInteraction, receiverParameter, producer,
            value, tag, messageTime, OrderType.TIMESTAMP, retraction.handle, sourceRegion);
         check(transportationName.equals(receiver.getTransportationTypeName(actual.transportation))
               && messageTime.equals(ownerRecorder.grants.get(0))
               && receiverTarget.equals(receiverRecorder.grants.get(0))
               && receiverRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "custom-transport regional interaction changed transport, grants, or callback order");

         receiver.disableTimeConstrained(); constrained = false;
         owner.disableTimeRegulation(); regulating = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = false;
         owner.unpublishInteractionClass(ownerInteraction); published = false;
         receiver.deleteRegion(receiverRegion); receiverRegion = null;
         owner.deleteRegion(sourceRegion); sourceRegion = null;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && ownerJoined) try { owner.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined) try {
            receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         } catch (Exception ignored) { }
         if (published && ownerJoined) try { owner.unpublishInteractionClass(ownerInteraction); }
            catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         deleteRegion(owner, sourceRegion, ownerJoined);
         if (receiverJoined) resign(receiver);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runRegionalAlternateAdvances(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      runAlternateAdvances(factory, fom, timeImplementation, interactionName, parameterName,
         firstDimensionName, secondDimensionName, callbackModel, true);
   }

   static void runDefaultRegionReenable(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-default-region-reenable-" + UUID.randomUUID();
      boolean ownerConnected = false, receiverConnected = false, created = false;
      boolean ownerJoined = false, receiverJoined = false, published = false, subscribed = false;
      boolean regulating = false, constrained = false;
      InteractionClassHandle ownerInteraction = null, receiverInteraction = null;
      RegionHandle receiverRegion = null;
      RegionHandleSet receiverRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel); receiverConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-default-region-reenable-owner", federation); ownerJoined = true;
         receiver.joinFederationExecution("java-tck-default-region-reenable-member", federation);
         receiverJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(receiverInteraction)
               && ownerParameter.equals(receiverParameter)
               && ownerFirst.equals(receiverFirst) && ownerSecond.equals(receiverSecond),
            "adapter DDM FOM returned inconsistent Time Constrained re-enable handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);
         checkDimensions(receiver, receiverInteraction, receiverFirst, receiverSecond);

         owner.publishInteractionClass(ownerInteraction); published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         DimensionHandleSet regionDimensions = receiver.getDimensionHandleSetFactory().create();
         regionDimensions.add(receiverFirst);
         receiverRegion = receiver.createRegion(regionDimensions);
         receiver.setRangeBounds(receiverRegion, receiverFirst, new RangeBounds(2, 6));
         receiverRegions = regionSet(receiver, receiverRegion);
         receiver.commitRegionModifications(receiverRegions);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = true;

         LogicalTimeFactory<?, ?> ownerFactory = owner.getTimeFactory();
         check(ownerFactory.getName().equals(receiver.getTimeFactory().getName()),
            "Time Constrained re-enable members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) ownerFactory.makeEpsilon();
         LogicalTimeInterval lookahead = epsilon;
         for (int i = 1; i < 5; ++i) lookahead = (LogicalTimeInterval) lookahead.add(epsilon);
         receiver.enableTimeConstrained(); constrained = true;
         await(callbackModel, receiver,
            () -> receiverRecorder.timeConstrainedEnabledCount >= 1,
            "initial timeConstrainedEnabled callback");
         owner.enableTimeRegulation(lookahead); regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabled,
            "timeRegulationEnabled callback");

         LogicalTime ownerInitial = owner.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime messageTime = addSteps(ownerInitial, epsilon, 6);
         LogicalTime publisherTarget = addSteps(ownerInitial, epsilon, 2);
         check(messageTime.equals(addSteps(receiverInitial,
               (LogicalTimeInterval) receiver.getTimeFactory().makeEpsilon(), 6)),
            "publisher and constrained receiver derived different message times");
         byte[] value = new byte[] {0x52, 0x45, 0x49, 0x4E};
         byte[] tag = new byte[] {0x44, 0x45, 0x46, 0x52};
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, value.clone());
         MessageRetractionReturn retraction = owner.sendInteraction(
            ownerInteraction, parameters, tag.clone(), messageTime);
         requireRetraction(retraction, "timestamped default-region re-enable interaction");
         check(receiverRecorder.interactions.isEmpty(),
            "default-region interaction delivered before the constrained frontier");

         receiver.disableTimeConstrained(); constrained = false;
         for (int i = 0; i < 8; ++i) pump(receiver, callbackModel);
         check(receiverRecorder.interactions.isEmpty(),
            "queued default-region interaction delivered while Time Constrained was disabled");
         receiver.enableTimeConstrained(); constrained = true;
         await(callbackModel, receiver,
            () -> receiverRecorder.timeConstrainedEnabledCount >= 2,
            "re-enabled timeConstrainedEnabled callback");
         receiverRecorder.callbackOrder.clear();
         ownerRecorder.callbackOrder.clear();
         receiver.timeAdvanceRequest(messageTime);
         owner.timeAdvanceRequest(publisherTarget);
         long deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline
               && (ownerRecorder.grants.size() != 1 || receiverRecorder.grants.size() != 1
                  || receiverRecorder.interactions.size() != 1)) {
            pump(owner, callbackModel);
            pump(receiver, callbackModel);
            Thread.sleep(1L);
         }
         check(ownerRecorder.grants.size() == 1 && receiverRecorder.grants.size() == 1
               && receiverRecorder.interactions.size() == 1,
            "re-enabled constrained interaction delivery or grant timed out");
         check(receiverRecorder.timeConstrainedEnabledCount == 2
               && receiverRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "re-enable returned wrong callback count or delivered after the grant");
         Interaction actual = receiverRecorder.interactions.get(0);
         check(actual.interaction.equals(receiverInteraction)
               && actual.parameters.size() == 1
               && Arrays.equals(value, actual.parameters.get(receiverParameter))
               && Arrays.equals(tag, actual.tag)
               && producer.equals(actual.producer)
               && messageTime.equals(actual.time)
               && actual.sentOrder == OrderType.TIMESTAMP
               && actual.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(actual.retraction)
               && actual.regions != null && actual.regions.isEmpty()
               && actual.transportation != null,
            "re-enabled default-region interaction returned incorrect callback metadata");
         check(!receiver.getTransportationTypeName(actual.transportation).isEmpty()
               && publisherTarget.equals(ownerRecorder.grants.get(0))
               && messageTime.equals(receiverRecorder.grants.get(0)),
            "re-enabled interaction returned unresolved transport or incorrect grant times");
         JavaTckSupport.expectFailure(() -> owner.retract(retraction.handle),
            "MessageCanNoLongerBeRetracted", "retracting delivered interaction after re-enable");

         receiver.disableTimeConstrained(); constrained = false;
         owner.disableTimeRegulation(); regulating = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = false;
         owner.unpublishInteractionClass(ownerInteraction); published = false;
         receiver.deleteRegion(receiverRegion); receiverRegion = null;
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions); }
            catch (Exception ignored) { }
         }
         if (published && ownerJoined) {
            try { owner.unpublishInteractionClass(ownerInteraction); } catch (Exception ignored) { }
         }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         if (receiverJoined) resign(receiver);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   static void runDefaultRegionRegulationReenable(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-default-region-regulation-reenable-" + UUID.randomUUID();
      boolean ownerConnected = false, receiverConnected = false, created = false;
      boolean ownerJoined = false, receiverJoined = false, published = false, subscribed = false;
      boolean regulating = false, constrained = false;
      InteractionClassHandle ownerInteraction = null, receiverInteraction = null;
      RegionHandle receiverRegion = null;
      RegionHandleSet receiverRegions = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel); receiverConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-default-region-regulation-owner", federation); ownerJoined = true;
         receiver.joinFederationExecution("java-tck-default-region-regulation-member", federation);
         receiverJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         check(ownerInteraction.equals(receiverInteraction)
               && ownerParameter.equals(receiverParameter)
               && ownerFirst.equals(receiverFirst) && ownerSecond.equals(receiverSecond),
            "adapter DDM FOM returned inconsistent regulation-reenable handles");
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);
         checkDimensions(receiver, receiverInteraction, receiverFirst, receiverSecond);

         owner.publishInteractionClass(ownerInteraction); published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         DimensionHandleSet regionDimensions = receiver.getDimensionHandleSetFactory().create();
         regionDimensions.add(receiverFirst);
         receiverRegion = receiver.createRegion(regionDimensions);
         receiver.setRangeBounds(receiverRegion, receiverFirst, new RangeBounds(2, 6));
         receiverRegions = regionSet(receiver, receiverRegion);
         receiver.commitRegionModifications(receiverRegions);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = true;

         LogicalTimeFactory<?, ?> ownerFactory = owner.getTimeFactory();
         check(ownerFactory.getName().equals(receiver.getTimeFactory().getName()),
            "regulation-reenable members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) ownerFactory.makeEpsilon();
         LogicalTimeInterval initialLookahead = intervalSteps(epsilon, 5);
         LogicalTimeInterval changedLookahead = intervalSteps(epsilon, 3);
         receiver.enableTimeConstrained(); constrained = true;
         await(callbackModel, receiver, () -> receiverRecorder.timeConstrainedEnabled,
            "regulation-reenable timeConstrainedEnabled callback");
         owner.enableTimeRegulation(initialLookahead); regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabledCount >= 1,
            "initial timeRegulationEnabled callback");

         LogicalTime ownerInitial = owner.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         check(ownerInitial.equals(receiverInitial),
            "regulation-reenable members began at different logical times");
         LogicalTime messageTime = addSteps(ownerInitial, epsilon, 5);
         LogicalTime publisherTarget = addSteps(ownerInitial, epsilon, 2);
         byte[] value = new byte[] {0x52, 0x45, 0x47, 0x44};
         byte[] tag = new byte[] {0x44, 0x45, 0x46, 0x52};
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, value.clone());
         MessageRetractionReturn retraction = owner.sendInteraction(
            ownerInteraction, parameters, tag.clone(), messageTime);
         requireRetraction(retraction, "timestamped default-region regulation-reenable interaction");
         check(receiverRecorder.interactions.isEmpty(),
            "regulation-reenable interaction delivered before its grant");

         owner.disableTimeRegulation(); regulating = false;
         for (int i = 0; i < 8; ++i) pump(owner, callbackModel);
         owner.enableTimeRegulation(changedLookahead); regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabledCount >= 2,
            "changed-lookahead timeRegulationEnabled callback");
         check(changedLookahead.equals(owner.queryLookahead()),
            "Query Lookahead did not return the changed interval");
         pump(receiver, callbackModel);
         check(receiverRecorder.interactions.isEmpty(),
            "queued interaction delivered during Time Regulation re-enable");

         receiverRecorder.callbackOrder.clear();
         ownerRecorder.callbackOrder.clear();
         receiver.timeAdvanceRequest(messageTime);
         owner.timeAdvanceRequest(publisherTarget);
         long deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline
               && (ownerRecorder.grants.size() != 1 || receiverRecorder.grants.size() != 1
                  || receiverRecorder.interactions.size() != 1)) {
            pump(owner, callbackModel);
            pump(receiver, callbackModel);
            Thread.sleep(1L);
         }
         check(ownerRecorder.grants.size() == 1 && receiverRecorder.grants.size() == 1
               && receiverRecorder.interactions.size() == 1,
            "regulation-reenabled interaction delivery or grants timed out");
         check(ownerRecorder.timeRegulationEnabledCount == 2
               && receiverRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "regulation re-enable returned wrong callback count or delivery/grant order");
         Interaction actual = receiverRecorder.interactions.get(0);
         check(actual.interaction.equals(receiverInteraction)
               && actual.parameters.size() == 1
               && Arrays.equals(value, actual.parameters.get(receiverParameter))
               && Arrays.equals(tag, actual.tag)
               && producer.equals(actual.producer)
               && messageTime.equals(actual.time)
               && actual.sentOrder == OrderType.TIMESTAMP
               && actual.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(actual.retraction)
               && actual.regions != null && actual.regions.isEmpty()
               && actual.transportation != null,
            "regulation-reenabled interaction returned incorrect callback metadata");
         check(!receiver.getTransportationTypeName(actual.transportation).isEmpty()
               && publisherTarget.equals(ownerRecorder.grants.get(0))
               && messageTime.equals(receiverRecorder.grants.get(0)),
            "regulation-reenabled interaction returned unresolved transport or wrong grant times");
         JavaTckSupport.expectFailure(() -> owner.retract(retraction.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting delivered interaction after Time Regulation re-enable");

         receiver.disableTimeConstrained(); constrained = false;
         owner.disableTimeRegulation(); regulating = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = false;
         owner.unpublishInteractionClass(ownerInteraction); published = false;
         receiver.deleteRegion(receiverRegion); receiverRegion = null;
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions); }
            catch (Exception ignored) { }
         }
         if (published && ownerJoined) {
            try { owner.unpublishInteractionClass(ownerInteraction); } catch (Exception ignored) { }
         }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         if (receiverJoined) resign(receiver);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void runAlternateAdvances(RtiFactory factory, String fom,
         String timeImplementation, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel,
         boolean regionalSource) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador flushReceiver = factory.getRtiAmbassador();
      RTIambassador taraReceiver = factory.getRtiAmbassador();
      RTIambassador nmraReceiver = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder flushRecorder = new Recorder();
      Recorder taraRecorder = new Recorder();
      Recorder nmraRecorder = new Recorder();
      RTIambassador[] receivers = {flushReceiver, taraReceiver, nmraReceiver};
      Recorder[] receiverRecorders = {flushRecorder, taraRecorder, nmraRecorder};
      String federation = "java-tck-timestamped-interaction-alternates-" + UUID.randomUUID();
      boolean ownerConnected = false, flushConnected = false;
      boolean taraConnected = false, nmraConnected = false, created = false;
      boolean ownerJoined = false, flushJoined = false, taraJoined = false, nmraJoined = false;
      boolean published = false, regulating = false;
      boolean flushPublished = false, flushRegulating = false;
      boolean[] subscribed = new boolean[receivers.length];
      boolean[] constrained = new boolean[receivers.length];
      InteractionClassHandle ownerInteraction = null;
      InteractionClassHandle[] receiverInteractions = new InteractionClassHandle[receivers.length];
      RegionHandle sourceRegion = null;
      RegionHandleSet sourceRegionSet = null;
      RegionHandle[] regions = new RegionHandle[receivers.length];
      RegionHandleSet[] regionSets = new RegionHandleSet[receivers.length];
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel); ownerConnected = true;
         flushReceiver.connect(flushRecorder.proxy(), callbackModel); flushConnected = true;
         taraReceiver.connect(taraRecorder.proxy(), callbackModel); taraConnected = true;
         nmraReceiver.connect(nmraRecorder.proxy(), callbackModel); nmraConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-default-region-alternates-owner", federation); ownerJoined = true;
         flushReceiver.joinFederationExecution("java-tck-default-region-fqr", federation);
         flushJoined = true;
         taraReceiver.joinFederationExecution("java-tck-default-region-tara", federation);
         taraJoined = true;
         nmraReceiver.joinFederationExecution("java-tck-default-region-nmra", federation);
         nmraJoined = true;

         ownerInteraction = owner.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         DimensionHandle ownerFirst = owner.getDimensionHandle(firstDimensionName);
         DimensionHandle ownerSecond = owner.getDimensionHandle(secondDimensionName);
         checkDimensions(owner, ownerInteraction, ownerFirst, ownerSecond);
         for (int i = 0; i < receivers.length; ++i) {
            RTIambassador receiver = receivers[i];
            receiverInteractions[i] = receiver.getInteractionClassHandle(interactionName);
            ParameterHandle receiverParameter = receiver.getParameterHandle(
               receiverInteractions[i], parameterName);
            DimensionHandle first = receiver.getDimensionHandle(firstDimensionName);
            DimensionHandle second = receiver.getDimensionHandle(secondDimensionName);
            check(ownerInteraction.equals(receiverInteractions[i])
                  && ownerParameter.equals(receiverParameter)
                  && ownerFirst.equals(first) && ownerSecond.equals(second),
               "adapter DDM FOM returned inconsistent alternate-interaction handles");
            checkDimensions(receiver, receiverInteractions[i], first, second);
         }
         owner.publishInteractionClass(ownerInteraction); published = true;
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         if (regionalSource) {
            sourceRegion = createRegion(owner, ownerFirst, ownerSecond, 1, 5, 1, 5);
            sourceRegionSet = regionSet(owner, sourceRegion);
         }
         for (int i = 0; i < receivers.length; ++i) {
            RTIambassador receiver = receivers[i];
            DimensionHandle first = receiver.getDimensionHandle(firstDimensionName);
            DimensionHandle second = receiver.getDimensionHandle(secondDimensionName);
            regions[i] = createRegion(receiver, first, second, 2, 6, 2, 6);
            regionSets[i] = regionSet(receiver, regions[i]);
            receiver.setConveyRegionDesignatorSetsSwitch(true);
            check(receiver.getConveyRegionDesignatorSetsSwitch(),
               "recipient did not enable region-designator callbacks");
            receiver.subscribeInteractionClassWithRegions(receiverInteractions[i], regionSets[i]);
            subscribed[i] = true;
         }

         LogicalTimeFactory<?, ?> ownerFactory = owner.getTimeFactory();
         String factoryName = ownerFactory.getName();
         for (RTIambassador receiver : receivers) {
            check(factoryName.equals(receiver.getTimeFactory().getName()),
               "timestamped interaction alternate members selected different time factories");
         }
         LogicalTimeInterval epsilon = (LogicalTimeInterval) ownerFactory.makeEpsilon();
         LogicalTimeInterval ownerLookahead = intervalSteps(epsilon, regionalSource ? 5 : 1);
         if (regionalSource) {
            flushReceiver.publishInteractionClass(receiverInteractions[0]);
            flushReceiver.changeInteractionOrderType(receiverInteractions[0], OrderType.TIMESTAMP);
            flushPublished = true;
         }
         for (int i = 0; i < receivers.length; ++i) {
            receivers[i].enableTimeConstrained(); constrained[i] = true;
            final int index = i;
            await(callbackModel, receivers[i],
               () -> receiverRecorders[index].timeConstrainedEnabled,
               "alternate timeConstrainedEnabled callback");
         }
         if (regionalSource) {
            flushReceiver.enableTimeRegulation(epsilon); flushRegulating = true;
            await(callbackModel, flushReceiver, () -> flushRecorder.timeRegulationEnabled,
               "Flush Queue receiver timeRegulationEnabled callback");
         }
         owner.enableTimeRegulation(ownerLookahead); regulating = true;
         await(callbackModel, owner, () -> ownerRecorder.timeRegulationEnabled,
            "alternate timeRegulationEnabled callback");

         LogicalTime initial = owner.queryLogicalTime();
         LogicalTime[] receiverInitials = new LogicalTime[receivers.length];
         for (int i = 0; i < receivers.length; ++i) {
            receiverInitials[i] = receivers[i].queryLogicalTime();
            check(initial.equals(receiverInitials[i]),
               "timestamped interaction alternate members began at different times");
         }
         LogicalTime messageTime = addSteps(initial, epsilon, regionalSource ? 7 : 3);
         LogicalTime publisherTarget = addSteps(initial, epsilon, 2);
         LogicalTime requestBoundary = addSteps(initial, epsilon, regionalSource ? 10 : 5);
         byte[] value = new byte[] {0x41, 0x56, 0x41, 0x2D, 0x52};
         byte[] tag = new byte[] {0x41, 0x56, 0x41, 0x2D, 0x44};
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, value.clone());
         MessageRetractionReturn retraction = regionalSource
            ? owner.sendInteractionWithRegions(
               ownerInteraction, parameters, sourceRegionSet, tag.clone(), messageTime)
            : owner.sendInteraction(ownerInteraction, parameters, tag.clone(), messageTime);
         requireRetraction(retraction, "timestamped interaction alternate");
         for (Recorder recorder : receiverRecorders) {
            check(recorder.interactions.isEmpty(),
               "timestamped interaction alternate delivered before an advance request");
         }
         for (Recorder recorder : receiverRecorders) recorder.callbackOrder.clear();
         ownerRecorder.callbackOrder.clear();

         flushReceiver.flushQueueRequest(requestBoundary);
         if (regionalSource) {
            await(callbackModel, flushReceiver,
               () -> flushRecorder.interactions.size() == 1
                  && flushRecorder.flushQueueGrantTimes.size() == 1,
               "distinct regional Flush Queue grant and interaction");
            LogicalTime flushGrant = flushRecorder.flushQueueGrantTimes.get(0);
            LogicalTime flushOptimistic = flushRecorder.flushQueueOptimisticTime;
            check(addSteps(initial, epsilon, 5).equals(flushGrant)
                  && messageTime.equals(flushOptimistic),
               "regional Flush Queue did not expose the distinct actual/optimistic frontier");
            LogicalTimeInterval receiverLookahead = flushReceiver.queryLookahead();
            check(epsilon.equals(receiverLookahead),
               "regional Flush Queue receiver returned the wrong lookahead");
            LogicalTime postGrantTime = (LogicalTime) flushGrant.add(receiverLookahead);
            DimensionHandle flushFirst = flushReceiver.getDimensionHandle(firstDimensionName);
            DimensionHandle flushSecond = flushReceiver.getDimensionHandle(secondDimensionName);
            RegionHandle postGrantRegion = createRegion(
               flushReceiver, flushFirst, flushSecond, 8, 9, 8, 9);
            RegionHandleSet postGrantRegions = regionSet(flushReceiver, postGrantRegion);
            ParameterHandle flushParameter = flushReceiver.getParameterHandle(
               receiverInteractions[0], parameterName);
            ParameterHandleValueMap postGrantValues =
               flushReceiver.getParameterHandleValueMapFactory().create(1);
            postGrantValues.put(flushParameter, value.clone());
            MessageRetractionReturn postGrantRetraction = flushReceiver.sendInteractionWithRegions(
               receiverInteractions[0], postGrantValues, postGrantRegions, tag.clone(), postGrantTime);
            requireRetraction(postGrantRetraction,
               "regional post-grant send at actual time plus lookahead");
            check(flushGrant.equals(flushReceiver.queryLogicalTime()),
               "regional post-grant send changed the Flush Queue receiver logical time");
            flushReceiver.deleteRegion(postGrantRegion);
         }
         taraReceiver.timeAdvanceRequestAvailable(messageTime);
         nmraReceiver.nextMessageRequestAvailable(requestBoundary);
         owner.timeAdvanceRequest(publisherTarget);
         long deadline = System.nanoTime() + 10_000_000_000L;
         while (System.nanoTime() < deadline) {
            for (RTIambassador ambassador : new RTIambassador[] {
                  owner, flushReceiver, taraReceiver, nmraReceiver}) pump(ambassador, callbackModel);
            if (ownerRecorder.grants.size() == 1
                  && flushRecorder.interactions.size() == 1
                  && flushRecorder.flushQueueGrantTimes.size() == 1
                  && taraRecorder.interactions.size() == 1 && taraRecorder.grants.size() == 1
                  && nmraRecorder.interactions.size() == 1 && nmraRecorder.grants.size() == 1) break;
            Thread.sleep(1L);
         }
         check(ownerRecorder.grants.size() == 1
               && flushRecorder.interactions.size() == 1
               && flushRecorder.flushQueueGrantTimes.size() == 1
               && taraRecorder.interactions.size() == 1 && taraRecorder.grants.size() == 1
               && nmraRecorder.interactions.size() == 1 && nmraRecorder.grants.size() == 1,
            "timestamped interaction alternate deliveries or grants timed out");

         check(flushRecorder.grants.isEmpty() && taraRecorder.flushQueueGrantTimes.isEmpty()
               && nmraRecorder.flushQueueGrantTimes.isEmpty(),
            "timestamped interaction alternate advances mixed grant types");
         check(flushRecorder.callbackOrder.equals(Arrays.asList(
                  "receiveInteraction", "flushQueueGrant"))
               && taraRecorder.callbackOrder.equals(Arrays.asList(
                  "receiveInteraction", "timeAdvanceGrant"))
               && nmraRecorder.callbackOrder.equals(Arrays.asList(
                  "receiveInteraction", "timeAdvanceGrant")),
            "timestamped interaction alternate granted before interaction delivery");
         for (int i = 0; i < receivers.length; ++i) {
            Interaction actual = receiverRecorders[i].interactions.get(0);
            ParameterHandle receiverParameter = receivers[i].getParameterHandle(
               receiverInteractions[i], parameterName);
            check(actual.interaction.equals(receiverInteractions[i])
                  && actual.parameters.size() == 1
                  && Arrays.equals(value, actual.parameters.get(receiverParameter))
                  && Arrays.equals(tag, actual.tag)
                  && producer.equals(actual.producer)
                  && messageTime.equals(actual.time)
                  && actual.sentOrder == OrderType.TIMESTAMP
                  && actual.receivedOrder == OrderType.TIMESTAMP
                  && retraction.handle.equals(actual.retraction)
                  && actual.regions != null
                  && (regionalSource
                     ? actual.regions.size() == 1 && actual.regions.contains(sourceRegion)
                     : actual.regions.isEmpty())
                  && actual.transportation != null,
               "timestamped interaction alternate returned incorrect interaction metadata");
            check(!receivers[i].getTransportationTypeName(actual.transportation).isEmpty(),
               "timestamped interaction alternate returned an unresolved transportation");
         }
         check(publisherTarget.equals(ownerRecorder.grants.get(0))
               && messageTime.equals(taraRecorder.grants.get(0))
               && messageTime.equals(nmraRecorder.grants.get(0)),
            "timestamped interaction alternate returned the wrong TAR/TARA/NMRA grant");
         LogicalTime flushGrant = flushRecorder.flushQueueGrantTimes.get(0);
         LogicalTime flushOptimistic = flushRecorder.flushQueueOptimisticTime;
         check(flushGrant != null && flushOptimistic != null
               && messageTime.equals(flushOptimistic)
               && compareTimes(flushGrant, initial) >= 0
               && compareTimes(flushGrant, requestBoundary) <= 0
               && compareTimes(flushOptimistic, flushGrant) >= 0
               && flushGrant.equals(flushReceiver.queryLogicalTime()),
            "Flush Queue Grant/query returned inconsistent actual or optimistic time");
         check(messageTime.equals(taraReceiver.queryLogicalTime())
               && messageTime.equals(nmraReceiver.queryLogicalTime())
               && publisherTarget.equals(owner.queryLogicalTime()),
            "alternate logical-time queries disagreed with their grants");

         LogicalTime past = addSteps(initial, epsilon, 1);
         JavaTckSupport.expectFailure(() -> flushReceiver.flushQueueRequest(past),
            "LogicalTimeAlreadyPassed", "interaction alternate FQR past-time request");
         JavaTckSupport.expectFailure(() -> taraReceiver.timeAdvanceRequestAvailable(past),
            "LogicalTimeAlreadyPassed", "interaction alternate TARA past-time request");
         JavaTckSupport.expectFailure(() -> nmraReceiver.nextMessageRequestAvailable(past),
            "LogicalTimeAlreadyPassed", "interaction alternate NMRA past-time request");
         JavaTckSupport.expectFailure(() -> owner.retract(retraction.handle),
            "MessageCanNoLongerBeRetracted", "retracting delivered default-region interaction");

         for (int i = 0; i < receivers.length; ++i) {
            receivers[i].disableTimeConstrained(); constrained[i] = false;
         }
         if (flushRegulating) {
            flushReceiver.disableTimeRegulation(); flushRegulating = false;
         }
         owner.disableTimeRegulation(); regulating = false;
         for (int i = 0; i < receivers.length; ++i) {
            receivers[i].unsubscribeInteractionClassWithRegions(receiverInteractions[i], regionSets[i]);
            subscribed[i] = false;
         }
         owner.unpublishInteractionClass(ownerInteraction); published = false;
         if (flushPublished) {
            flushReceiver.unpublishInteractionClass(receiverInteractions[0]);
            flushPublished = false;
         }
         for (int i = 0; i < receivers.length; ++i) {
            receivers[i].deleteRegion(regions[i]); regions[i] = null;
         }
         deleteRegion(owner, sourceRegion, ownerJoined);
         sourceRegion = null;
      } finally {
         if (flushRegulating && flushJoined) try { flushReceiver.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (flushPublished && flushJoined) try {
            flushReceiver.unpublishInteractionClass(receiverInteractions[0]);
         } catch (Exception ignored) { }
         for (int i = 0; i < receivers.length; ++i) {
            if (constrained[i]) try { receivers[i].disableTimeConstrained(); }
               catch (Exception ignored) { }
            if (subscribed[i]) try {
               receivers[i].unsubscribeInteractionClassWithRegions(
                  receiverInteractions[i], regionSets[i]);
            } catch (Exception ignored) { }
            deleteRegion(receivers[i], regions[i], i == 0 ? flushJoined : i == 1 ? taraJoined : nmraJoined);
         }
         deleteRegion(owner, sourceRegion, ownerJoined);
         if (regulating && ownerJoined) try { owner.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (published && ownerJoined) try { owner.unpublishInteractionClass(ownerInteraction); }
            catch (Exception ignored) { }
         if (nmraJoined) resign(nmraReceiver);
         if (taraJoined) resign(taraReceiver);
         if (flushJoined) resign(flushReceiver);
         if (ownerJoined) resign(owner);
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (nmraConnected) disconnect(nmraReceiver);
         if (taraConnected) disconnect(taraReceiver);
         if (flushConnected) disconnect(flushReceiver);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void checkDimensions(RTIambassador ambassador,
         InteractionClassHandle interaction, DimensionHandle first, DimensionHandle second)
         throws Exception {
      Set<DimensionHandle> available = ambassador.getAvailableDimensionsForInteractionClass(interaction);
      check(available.contains(first) && available.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 9
            && ambassador.getDimensionUpperBound(second) >= 9,
         "adapter DDM FOM interaction does not expose configured dimension ranges");
   }

   private static RegionHandle createRegion(RTIambassador ambassador,
         DimensionHandle first, DimensionHandle second, long firstLower, long firstUpper,
         long secondLower, long secondUpper) throws Exception {
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

   private static MessageRetractionReturn send(RTIambassador owner,
         InteractionClassHandle interaction, ParameterHandle parameter,
         RegionHandleSet regions, byte[] value, byte[] tag, LogicalTime<?, ?> timestamp)
         throws Exception {
      ParameterHandleValueMap values = owner.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value.clone());
      return owner.sendInteractionWithRegions(interaction, values, regions, tag.clone(), timestamp);
   }

   private static void assertInteraction(RTIambassador receiver, Interaction actual,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedValue, byte[] expectedTag,
         LogicalTime<?, ?> expectedTime, OrderType expectedReceivedOrder,
         MessageRetractionHandle expectedRetraction, RegionHandle expectedRegion)
         throws Exception {
      check(actual.interaction.equals(expectedClass)
            && actual.parameters.size() == 1
            && Arrays.equals(expectedValue, actual.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, actual.tag)
            && expectedProducer.equals(actual.producer)
            && expectedTime.equals(actual.time)
            && actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == expectedReceivedOrder
            && expectedRetraction.equals(actual.retraction)
            && actual.regions != null && actual.regions.size() == 1
            && actual.regions.contains(expectedRegion)
            && actual.transportation != null,
         "timestamped regional interaction returned incorrect standard callback metadata");
      check(!receiver.getTransportationTypeName(actual.transportation).isEmpty()
            && actual.time.toString() != null && !actual.time.toString().isEmpty(),
         "timestamped regional interaction returned unresolvable transportation or time metadata");
   }

   private static void requireRetraction(MessageRetractionReturn value, String description) {
      check(value != null && value.retractionHandleIsValid && value.handle != null,
         description + " returned no valid message-retraction handle");
   }

   private static LogicalTime addSteps(LogicalTime start, LogicalTimeInterval interval,
         int count) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < count; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTimeInterval intervalSteps(LogicalTimeInterval interval, int count)
         throws Exception {
      LogicalTimeInterval result = interval;
      for (int i = 1; i < count; ++i) {
         result = (LogicalTimeInterval) result.add(interval);
      }
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compareTimes(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   private static void await(CallbackModel model, RTIambassador ambassador,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) pump(ambassador, model);
      check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         for (int i = 0; i < 8; ++i) ambassador.evokeCallback(0.0);
      } else {
         Thread.yield();
      }
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean joined) {
      if (region != null && joined) {
         try { ambassador.deleteRegion(region); } catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK timestamped region owner";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == args[0];
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

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final Set<RegionHandle> regions;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Interaction(Object[] args) {
         interaction = (InteractionClassHandle) args[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) args[1];
         values.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) args[2]).clone();
         transportation = (TransportationTypeHandle) args[3];
         producer = (FederateHandle) args[4];
         regions = args[5] == null ? null : new HashSet<>((RegionHandleSet) args[5]);
         time = (LogicalTime<?, ?>) args[6];
         sentOrder = (OrderType) args[7];
         receivedOrder = (OrderType) args[8];
         retraction = (MessageRetractionHandle) args[9];
      }
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile int timeRegulationEnabledCount;
      private volatile boolean timeConstrainedEnabled;
      private volatile int timeConstrainedEnabledCount;
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile LogicalTime<?, ?> flushQueueGrantTime;
      private volatile LogicalTime<?, ?> flushQueueOptimisticTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
                  timeRegulationEnabledCount++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
                  timeConstrainedEnabledCount++;
               } else if ("receiveInteraction".equals(name)
                     && args != null && args.length == 10) {
                  interactions.add(new Interaction(args));
                  callbackOrder.add(name);
               } else if ("requestRetraction".equals(name) && args != null
                     && args.length == 1) {
                  retractions.add((MessageRetractionHandle) args[0]);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)
                     && args != null && args.length == 1) {
                  grants.add((LogicalTime<?, ?>) args[0]);
                  callbackOrder.add(name);
                } else if ("flushQueueGrant".equals(name)
                      && args != null && args.length == 2) {
                   flushQueueGrantTime = (LogicalTime<?, ?>) args[0];
                   flushQueueOptimisticTime = (LogicalTime<?, ?>) args[1];
                   flushQueueGrantTimes.add(flushQueueGrantTime);
                   callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped region callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
