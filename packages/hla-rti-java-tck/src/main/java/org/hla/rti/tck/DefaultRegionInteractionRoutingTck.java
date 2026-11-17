package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Resolves ordinary interaction delivery through explicit and implicit default regions. */
final class DefaultRegionInteractionRoutingTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private DefaultRegionInteractionRoutingTck() { }

   static void run(RtiFactory factory, String fom, String interactionName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador regional = factory.getRtiAmbassador();
      RTIambassador mixed = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder regionalCallbacks = new Recorder();
      Recorder mixedCallbacks = new Recorder();
      String federation = "java-tck-default-region-interaction-routing-" + UUID.randomUUID();
      boolean publisherConnected = false, regionalConnected = false, mixedConnected = false;
      boolean created = false, publisherJoined = false, regionalJoined = false, mixedJoined = false;
      boolean published = false, publisherOrdinarySubscribed = false;
      boolean publisherRegionalSubscribed = false, regionalSubscribed = false;
      boolean mixedOrdinarySubscribed = false;
      boolean mixedRegionalSubscribed = false;
      RegionHandle sourceRegion = null, publisherSubscriptionRegion = null;
      RegionHandle regionalRegion = null, mixedRegion = null;
      InteractionClassHandle publisherInteraction = null, regionalInteraction = null;
      InteractionClassHandle mixedInteraction = null;
      ParameterHandle publisherParameter = null, regionalParameter = null, mixedParameter = null;
      RegionHandleSet sourceRegions = null, regionalRegions = null, mixedRegions = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         regional.connect(regionalCallbacks.proxy(), callbackModel);
         regionalConnected = true;
         mixed.connect(mixedCallbacks.proxy(), callbackModel);
         mixedConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producingFederate = publisher.joinFederationExecution(
            "java-tck-default-region-publisher", federation);
         publisherJoined = true;
         regional.joinFederationExecution("java-tck-default-region-regional", federation);
         regionalJoined = true;
         mixed.joinFederationExecution("java-tck-default-region-mixed", federation);
         mixedJoined = true;
         publisherCallbacks.inspectRegionDesignatorsWith(publisher);
         regionalCallbacks.inspectRegionDesignatorsWith(regional);
         mixedCallbacks.inspectRegionDesignatorsWith(mixed);

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         regionalInteraction = regional.getInteractionClassHandle(interactionName);
         mixedInteraction = mixed.getInteractionClassHandle(interactionName);
         publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         regionalParameter = regional.getParameterHandle(regionalInteraction, parameterName);
         mixedParameter = mixed.getParameterHandle(mixedInteraction, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle regionalFirst = regional.getDimensionHandle(firstDimensionName);
         DimensionHandle regionalSecond = regional.getDimensionHandle(secondDimensionName);
         DimensionHandle mixedFirst = mixed.getDimensionHandle(firstDimensionName);
         DimensionHandle mixedSecond = mixed.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherInteraction.equals(regionalInteraction)
               && publisherInteraction.equals(mixedInteraction)
               && publisherParameter.equals(regionalParameter)
               && publisherParameter.equals(mixedParameter)
               && publisherFirst.equals(regionalFirst) && publisherSecond.equals(regionalSecond)
               && publisherFirst.equals(mixedFirst) && publisherSecond.equals(mixedSecond),
            "adapter DDM FOM handles changed across default-region interaction federates");
         checkDimensions(publisher, publisherInteraction, publisherFirst, publisherSecond);
         checkDimensions(regional, regionalInteraction, regionalFirst, regionalSecond);
         checkDimensions(mixed, mixedInteraction, mixedFirst, mixedSecond);

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 0, 1);
         publisherSubscriptionRegion = createRegion(
            publisher, publisherFirst, publisherSecond, 0, 1);
         regionalRegion = createRegion(regional, regionalFirst, regionalSecond, 0, 1);
         mixedRegion = createRegion(mixed, mixedFirst, mixedSecond, 2, 3);
         sourceRegions = regionSet(publisher, sourceRegion);
         RegionHandleSet publisherSubscriptionRegions =
            regionSet(publisher, publisherSubscriptionRegion);
         regionalRegions = regionSet(regional, regionalRegion);
         mixedRegions = regionSet(mixed, mixedRegion);

         regional.setConveyRegionDesignatorSetsSwitch(true);
         mixed.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(regional.getConveyRegionDesignatorSetsSwitch()
               && mixed.getConveyRegionDesignatorSetsSwitch(),
            "default-region receivers did not enable region designator callbacks");
         regional.subscribeInteractionClassWithRegions(regionalInteraction, regionalRegions);
         regionalSubscribed = true;
         mixed.subscribeInteractionClass(mixedInteraction);
         mixedOrdinarySubscribed = true;
         mixed.subscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         mixedRegionalSubscribed = true;
         List<RegionDesignatorSnapshot> sourceDesignators =
            snapshotRegionDesignators(publisher, sourceRegions);

         byte[] firstValue = new byte[] {(byte) 0xD1, 0x04};
         byte[] firstTag = new byte[] {0x01, 0x02};
         sendRegional(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, firstValue), sourceRegions, firstTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.interactions.size() >= 1
               && mixedCallbacks.interactions.size() >= 1,
            "explicit regional send beside a disjoint regional subscription");
         assertInteractionWithRegionDesignators(regional,
            regionalCallbacks.interactions.get(0), regionalInteraction, regionalParameter,
            producingFederate, firstValue, firstTag, sourceDesignators,
            "overlapping regional delivery");
         pumpSeveral(regional, mixed, callbackModel);
         JavaTckSupport.check(regionalCallbacks.interactions.size() == 1
               && mixedCallbacks.interactions.size() == 1,
            "ordinary interaction subscription was shadowed by a disjoint regional declaration");
         assertInteractionMetadata(mixed, mixedCallbacks.interactions.get(0), mixedInteraction,
            mixedParameter, producingFederate, firstValue, firstTag,
            "ordinary delivery beside a disjoint regional declaration");
         assertNoLoopback(publisher, publisherCallbacks, callbackModel,
            "initial regional send without publisher subscriptions");
         publisher.subscribeInteractionClass(publisherInteraction);
         publisherOrdinarySubscribed = true;
         publisher.subscribeInteractionClassWithRegions(
            publisherInteraction, publisherSubscriptionRegions);
         publisherRegionalSubscribed = true;

         mixed.unsubscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         mixedRegionalSubscribed = false;
         byte[] secondValue = new byte[] {0x71, 0x72};
         byte[] secondTag = new byte[] {0x03, 0x04};
         sendRegional(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, secondValue), sourceRegions, secondTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.interactions.size() >= 2
               && mixedCallbacks.interactions.size() >= 2,
            "ordinary subscription restoring explicit regional delivery");
         assertNoLoopback(publisher, publisherCallbacks, callbackModel,
            "regional send after publisher subscriptions");
         JavaTckSupport.check(regionalCallbacks.interactions.size() == 2
               && mixedCallbacks.interactions.size() == 2,
            "ordinary subscription restoration produced an unexpected callback count");
         assertInteraction(regional, regionalCallbacks.interactions.get(1), regionalInteraction,
            regionalParameter, producingFederate, secondValue, secondTag, null,
            "regional receiver after explicit regional send");
         assertInteractionMetadata(mixed, mixedCallbacks.interactions.get(1), mixedInteraction,
            mixedParameter, producingFederate, secondValue, secondTag,
            "ordinary receiver after explicit regional send");

         mixed.subscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         mixedRegionalSubscribed = true;
         byte[] thirdValue = new byte[] {0x05, 0x06};
         byte[] thirdTag = new byte[] {0x07, 0x08};
         sendOrdinary(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, thirdValue), thirdTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.interactions.size() >= 3
               && mixedCallbacks.interactions.size() >= 3,
            "ordinary send through the implicit default region");
         JavaTckSupport.check(regionalCallbacks.interactions.size() == 3
               && mixedCallbacks.interactions.size() == 3,
            "ordinary default-region send produced an unexpected callback count");
         assertInteraction(regional, regionalCallbacks.interactions.get(2), regionalInteraction,
            regionalParameter, producingFederate, thirdValue, thirdTag,
            java.util.Collections.emptySet(), "regional receiver after ordinary send");
         assertInteraction(mixed, mixedCallbacks.interactions.get(2), mixedInteraction,
            mixedParameter, producingFederate, thirdValue, thirdTag,
            java.util.Collections.emptySet(), "mixed receiver after ordinary send");
         assertNoLoopback(publisher, publisherCallbacks, callbackModel,
            "ordinary send after publisher subscriptions");

         regional.setConveyRegionDesignatorSetsSwitch(false);
         JavaTckSupport.check(!regional.getConveyRegionDesignatorSetsSwitch(),
            "regional receiver did not disable region-designator callbacks");
         byte[] fourthValue = new byte[] {0x09, 0x0a};
         byte[] fourthTag = new byte[] {0x0b, 0x0c};
         sendRegional(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, fourthValue), sourceRegions, fourthTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.interactions.size() >= 4
               && mixedCallbacks.interactions.size() >= 4,
            "interaction delivery with per-federate region-designator switches");
         Interaction fourthRegional = regionalCallbacks.interactions.get(3);
         Interaction fourthMixed = mixedCallbacks.interactions.get(3);
         assertInteractionMetadata(regional, fourthRegional, regionalInteraction,
            regionalParameter, producingFederate, fourthValue, fourthTag,
            "regional receiver with region-designator switch disabled");
         assertInteractionMetadata(mixed, fourthMixed, mixedInteraction,
            mixedParameter, producingFederate, fourthValue, fourthTag,
            "mixed receiver with its region-designator switch enabled");
         JavaTckSupport.check(fourthRegional.regions == null
               && fourthMixed.regions != null && fourthMixed.regions.size() == 1,
            "one federate's disabled region-designator switch affected another receiver");

         regional.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(regional.getConveyRegionDesignatorSetsSwitch(),
            "regional receiver did not re-enable region-designator callbacks");
         byte[] fifthValue = new byte[] {0x0d, 0x0e};
         byte[] fifthTag = new byte[] {0x0f, 0x10};
         sendRegional(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, fifthValue), sourceRegions, fifthTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.interactions.size() >= 5
               && mixedCallbacks.interactions.size() >= 5,
            "interaction region designators after switch re-enable");
         JavaTckSupport.check(regionalCallbacks.interactions.size() == 5
               && mixedCallbacks.interactions.size() == 5,
            "switch re-enable produced an unexpected callback count");
         assertInteractionWithRegionDesignators(regional,
            regionalCallbacks.interactions.get(4), regionalInteraction, regionalParameter,
            producingFederate, fifthValue, fifthTag, sourceDesignators,
            "regional receiver after region-designator switch re-enable");

         mixed.unsubscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         mixedRegionalSubscribed = false;
         mixed.unsubscribeInteractionClass(mixedInteraction);
         mixedOrdinarySubscribed = false;
         regional.unsubscribeInteractionClassWithRegions(regionalInteraction, regionalRegions);
         regionalSubscribed = false;
         publisher.unsubscribeInteractionClassWithRegions(
            publisherInteraction, publisherSubscriptionRegions);
         publisherRegionalSubscribed = false;
         publisher.unsubscribeInteractionClass(publisherInteraction);
         publisherOrdinarySubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         publisher.deleteRegion(publisherSubscriptionRegion);
         publisherSubscriptionRegion = null;
         regional.deleteRegion(regionalRegion);
         regionalRegion = null;
         mixed.deleteRegion(mixedRegion);
         mixedRegion = null;
         mixed.resignFederationExecution(ResignAction.NO_ACTION);
         mixedJoined = false;
         regional.resignFederationExecution(ResignAction.NO_ACTION);
         regionalJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (mixedRegionalSubscribed && mixedJoined && mixedRegions != null) try {
            mixed.unsubscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         } catch (Exception ignored) { }
         if (mixedOrdinarySubscribed && mixedJoined && mixedInteraction != null) try {
            mixed.unsubscribeInteractionClass(mixedInteraction);
         } catch (Exception ignored) { }
         if (regionalSubscribed && regionalJoined && regionalRegions != null) try {
            regional.unsubscribeInteractionClassWithRegions(regionalInteraction, regionalRegions);
         } catch (Exception ignored) { }
         if (publisherRegionalSubscribed && publisherJoined && publisherInteraction != null
               && publisherSubscriptionRegion != null) try {
            publisher.unsubscribeInteractionClassWithRegions(
               publisherInteraction, regionSet(publisher, publisherSubscriptionRegion));
         } catch (Exception ignored) { }
         if (publisherOrdinarySubscribed && publisherJoined && publisherInteraction != null) try {
            publisher.unsubscribeInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         deleteRegion(publisher, sourceRegion, publisherJoined);
         deleteRegion(publisher, publisherSubscriptionRegion, publisherJoined);
         deleteRegion(regional, regionalRegion, regionalJoined);
         deleteRegion(mixed, mixedRegion, mixedJoined);
         if (mixedJoined) try { mixed.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (regionalJoined) try { regional.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (regionalConnected) try { regional.disconnect(); } catch (Exception ignored) { }
         if (mixedConnected) try { mixed.disconnect(); } catch (Exception ignored) { }
      }
   }

   static void runUnsubscribeIsolation(RtiFactory factory, String fom, String interactionName,
         String parameterName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador regional = factory.getRtiAmbassador();
      RTIambassador mixed = factory.getRtiAmbassador();
      Recorder regionalCallbacks = new Recorder();
      Recorder mixedCallbacks = new Recorder();
      String federation = "java-tck-default-region-interaction-unsubscribe-isolation-"
         + UUID.randomUUID();
      boolean publisherConnected = false, regionalConnected = false, mixedConnected = false;
      boolean created = false, publisherJoined = false, regionalJoined = false, mixedJoined = false;
      boolean published = false, regionalSubscribed = false, mixedOrdinarySubscribed = false;
      boolean mixedRegionalSubscribed = false;
      RegionHandle sourceRegion = null, disjointSourceRegion = null;
      RegionHandle regionalRegion = null, mixedRegion = null;
      InteractionClassHandle publisherInteraction = null, regionalInteraction = null;
      InteractionClassHandle mixedInteraction = null;
      ParameterHandle publisherParameter = null, regionalParameter = null, mixedParameter = null;
      RegionHandleSet regionalRegions = null, mixedRegions = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         regional.connect(regionalCallbacks.proxy(), callbackModel);
         regionalConnected = true;
         mixed.connect(mixedCallbacks.proxy(), callbackModel);
         mixedConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producingFederate = publisher.joinFederationExecution(
            "java-tck-default-region-interaction-publisher", federation);
         publisherJoined = true;
         regional.joinFederationExecution("java-tck-default-region-interaction-regional", federation);
         regionalJoined = true;
         mixed.joinFederationExecution("java-tck-default-region-interaction-mixed", federation);
         mixedJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         regionalInteraction = regional.getInteractionClassHandle(interactionName);
         mixedInteraction = mixed.getInteractionClassHandle(interactionName);
         publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         regionalParameter = regional.getParameterHandle(regionalInteraction, parameterName);
         mixedParameter = mixed.getParameterHandle(mixedInteraction, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle regionalFirst = regional.getDimensionHandle(firstDimensionName);
         DimensionHandle regionalSecond = regional.getDimensionHandle(secondDimensionName);
         DimensionHandle mixedFirst = mixed.getDimensionHandle(firstDimensionName);
         DimensionHandle mixedSecond = mixed.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherInteraction.equals(regionalInteraction)
               && publisherInteraction.equals(mixedInteraction)
               && publisherParameter.equals(regionalParameter)
               && publisherParameter.equals(mixedParameter)
               && publisherFirst.equals(regionalFirst) && publisherSecond.equals(regionalSecond)
               && publisherFirst.equals(mixedFirst) && publisherSecond.equals(mixedSecond),
            "adapter DDM FOM handles changed across interaction unsubscribe-isolation federates");
         checkDimensions(publisher, publisherInteraction, publisherFirst, publisherSecond);
         checkDimensions(regional, regionalInteraction, regionalFirst, regionalSecond);
         checkDimensions(mixed, mixedInteraction, mixedFirst, mixedSecond);

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 0, 1);
         disjointSourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 2, 3);
         regionalRegion = createRegion(regional, regionalFirst, regionalSecond, 0, 1);
         mixedRegion = createRegion(mixed, mixedFirst, mixedSecond, 2, 3);
         RegionHandleSet sourceRegions = regionSet(publisher, sourceRegion);
         RegionHandleSet disjointSourceRegions = regionSet(publisher, disjointSourceRegion);
         regionalRegions = regionSet(regional, regionalRegion);
         mixedRegions = regionSet(mixed, mixedRegion);

         regional.subscribeInteractionClassWithRegions(regionalInteraction, regionalRegions);
         regionalSubscribed = true;
         mixed.subscribeInteractionClass(mixedInteraction);
         mixedOrdinarySubscribed = true;
         mixed.subscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         mixedRegionalSubscribed = true;

         byte[] initialValue = new byte[] {0x31, 0x01};
         byte[] initialTag = new byte[] {0x41, 0x01};
         sendRegional(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, initialValue), sourceRegions, initialTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.interactions.size() >= 1 && mixedCallbacks.interactions.size() >= 1,
            "default-region and matching regional interaction subscriptions");
         pumpSeveral(regional, mixed, callbackModel);
         JavaTckSupport.check(regionalCallbacks.interactions.size() == 1
               && mixedCallbacks.interactions.size() == 1,
            "ordinary interaction subscription did not remain effective beside a disjoint regional declaration");
         assertUnsubscribeInteraction(regionalCallbacks.interactions.get(0),
            regionalInteraction, regionalParameter, producingFederate, initialValue, initialTag,
            "initial regional delivery");
         assertUnsubscribeInteraction(mixedCallbacks.interactions.get(0),
            mixedInteraction, mixedParameter, producingFederate, initialValue, initialTag,
            "initial ordinary delivery");

         mixed.unsubscribeInteractionClass(mixedInteraction);
         mixedOrdinarySubscribed = false;
         byte[] afterOrdinaryUnsubscribeValue = new byte[] {0x31, 0x02};
         byte[] afterOrdinaryUnsubscribeTag = new byte[] {0x41, 0x02};
         sendRegional(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, afterOrdinaryUnsubscribeValue),
            sourceRegions, afterOrdinaryUnsubscribeTag);
         await(regional, mixed, callbackModel,
            () -> regionalCallbacks.interactions.size() >= 2,
            "matching regional delivery after ordinary interaction unsubscribe");
         pumpSeveral(regional, mixed, callbackModel);
         JavaTckSupport.check(regionalCallbacks.interactions.size() == 2
               && mixedCallbacks.interactions.size() == 1,
            "ordinary interaction unsubscribe retained its default-region route or removed the disjoint regional declaration");
         assertUnsubscribeInteraction(regionalCallbacks.interactions.get(1),
            regionalInteraction, regionalParameter, producingFederate,
            afterOrdinaryUnsubscribeValue, afterOrdinaryUnsubscribeTag,
            "regional delivery after ordinary unsubscribe");

         byte[] regionalValue = new byte[] {0x31, 0x03};
         byte[] regionalTag = new byte[] {0x41, 0x03};
         sendRegional(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, regionalValue), disjointSourceRegions,
            regionalTag);
         await(regional, mixed, callbackModel,
            () -> mixedCallbacks.interactions.size() >= 2,
            "explicit regional delivery after ordinary interaction unsubscribe");
         pumpSeveral(regional, mixed, callbackModel);
         JavaTckSupport.check(mixedCallbacks.interactions.size() == 2
               && regionalCallbacks.interactions.size() == 2,
            "ordinary interaction unsubscribe removed the independent matching regional subscription");
         assertUnsubscribeInteraction(mixedCallbacks.interactions.get(1), mixedInteraction,
            mixedParameter, producingFederate, regionalValue, regionalTag,
            "disjoint regional delivery after ordinary unsubscribe");

         mixed.unsubscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         mixedRegionalSubscribed = false;
         regional.unsubscribeInteractionClassWithRegions(regionalInteraction, regionalRegions);
         regionalSubscribed = false;
         byte[] afterFinalUnsubscribeValue = new byte[] {0x31, 0x04};
         byte[] afterFinalUnsubscribeTag = new byte[] {0x41, 0x04};
         sendRegional(publisher, publisherInteraction,
            parameterMap(publisher, publisherParameter, afterFinalUnsubscribeValue), sourceRegions,
            afterFinalUnsubscribeTag);
         pumpSeveral(regional, mixed, callbackModel);
         JavaTckSupport.check(regionalCallbacks.interactions.size() == 2
               && mixedCallbacks.interactions.size() == 2,
            "an interaction subscription remained effective after all ordinary and regional declarations were removed");

         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         publisher.deleteRegion(disjointSourceRegion);
         disjointSourceRegion = null;
         regional.deleteRegion(regionalRegion);
         regionalRegion = null;
         mixed.deleteRegion(mixedRegion);
         mixedRegion = null;
         mixed.resignFederationExecution(ResignAction.NO_ACTION);
         mixedJoined = false;
         regional.resignFederationExecution(ResignAction.NO_ACTION);
         regionalJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (mixedRegionalSubscribed && mixedJoined && mixedRegions != null) try {
            mixed.unsubscribeInteractionClassWithRegions(mixedInteraction, mixedRegions);
         } catch (Exception ignored) { }
         if (mixedOrdinarySubscribed && mixedJoined && mixedInteraction != null) try {
            mixed.unsubscribeInteractionClass(mixedInteraction);
         } catch (Exception ignored) { }
         if (regionalSubscribed && regionalJoined && regionalRegions != null) try {
            regional.unsubscribeInteractionClassWithRegions(regionalInteraction, regionalRegions);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         deleteRegion(publisher, sourceRegion, publisherJoined);
         deleteRegion(publisher, disjointSourceRegion, publisherJoined);
         deleteRegion(regional, regionalRegion, regionalJoined);
         deleteRegion(mixed, mixedRegion, mixedJoined);
         if (mixedJoined) try { mixed.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (regionalJoined) try { regional.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (regionalConnected) try { regional.disconnect(); } catch (Exception ignored) { }
         if (mixedConnected) try { mixed.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador,
         InteractionClassHandle interaction, DimensionHandle first, DimensionHandle second)
         throws Exception {
      DimensionHandleSet available = ambassador.getAvailableDimensionsForInteractionClass(interaction);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 3
            && ambassador.getDimensionUpperBound(second) >= 3,
         "adapter interaction class must expose both dimensions and sufficient range");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      return regions;
   }

   private static ParameterHandleValueMap parameterMap(RTIambassador ambassador,
         ParameterHandle parameter, byte[] value) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, value.clone());
      return values;
   }

   private static void sendRegional(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandleValueMap parameters, RegionHandleSet regions, byte[] tag) throws Exception {
      publisher.sendInteractionWithRegions(interaction, parameters, regions, tag.clone());
   }

   private static void sendOrdinary(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandleValueMap parameters, byte[] tag) throws Exception {
      publisher.sendInteraction(interaction, parameters, tag.clone());
   }

   private static List<RegionDesignatorSnapshot> snapshotRegionDesignators(
         RTIambassador ambassador, RegionHandleSet regions) throws Exception {
      List<RegionDesignatorSnapshot> result = new ArrayList<>();
      for (RegionHandle region : regions) {
         Set<String> dimensions = new HashSet<>();
         Map<String, RangeSnapshot> ranges = new HashMap<>();
         for (DimensionHandle dimension : ambassador.getDimensionHandleSet(region)) {
            String name = ambassador.getDimensionName(dimension);
            RangeBounds bounds = ambassador.getRangeBounds(region, dimension);
            dimensions.add(name);
            ranges.put(name, new RangeSnapshot(bounds.lower, bounds.upper));
         }
         result.add(new RegionDesignatorSnapshot(dimensions, ranges));
      }
      return result;
   }

   private static void assertInteraction(RTIambassador receiver, Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedValue, byte[] expectedTag,
         Set<RegionHandle> expectedRegions, String description) throws Exception {
      assertInteractionMetadata(receiver, interaction, expectedClass, expectedParameter,
         expectedProducer, expectedValue, expectedTag, description);
      if (expectedRegions != null) {
         JavaTckSupport.check(interaction.regions != null
               && interaction.regions.equals(expectedRegions),
            description + " returned incorrect source-region metadata");
      }
   }

   private static void assertInteractionMetadata(RTIambassador receiver,
         Interaction interaction, InteractionClassHandle expectedClass,
         ParameterHandle expectedParameter, FederateHandle expectedProducer,
         byte[] expectedValue, byte[] expectedTag, String description) throws Exception {
      JavaTckSupport.check(expectedClass.equals(interaction.interaction)
            && interaction.parameters.size() == 1
            && Arrays.equals(expectedValue, interaction.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, interaction.tag)
            && expectedProducer.equals(interaction.producer),
         description + " changed class, parameter, payload, tag, or producer");
      JavaTckSupport.check(interaction.transportation != null
            && !receiver.getTransportationTypeName(interaction.transportation).isEmpty(),
         description + " returned an unresolvable transportation type");
   }

   private static void assertInteractionWithRegionDesignators(RTIambassador receiver,
         Interaction interaction, InteractionClassHandle expectedClass,
         ParameterHandle expectedParameter, FederateHandle expectedProducer,
         byte[] expectedValue, byte[] expectedTag,
         List<RegionDesignatorSnapshot> expectedDesignators, String description) throws Exception {
      assertInteractionMetadata(receiver, interaction, expectedClass, expectedParameter,
         expectedProducer, expectedValue, expectedTag, description);
      JavaTckSupport.check(interaction.regions != null
            && interaction.regions.size() == expectedDesignators.size()
            && interaction.regionDesignators != null
            && interaction.regionDesignators.size() == expectedDesignators.size()
            && expectedDesignators.equals(interaction.regionDesignators),
         description + " changed the receiver-local source-region dimensions or bounds");
   }

   private static void assertNoLoopback(RTIambassador publisher, Recorder callbacks,
         CallbackModel callbackModel, String description) throws Exception {
      long deadline = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(250);
      while (System.nanoTime() < deadline) {
         pump(publisher, callbackModel);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(callbacks.interactions.isEmpty(),
         description + " delivered an induced interaction callback to the sender");
   }

   private static void assertUnsubscribeInteraction(Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedValue, byte[] expectedTag,
         String description) {
      JavaTckSupport.check(expectedClass.equals(interaction.interaction)
            && interaction.parameters.size() == 1
            && Arrays.equals(expectedValue, interaction.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, interaction.tag)
            && expectedProducer.equals(interaction.producer),
         description + " changed class, parameter, payload, tag, or producer");
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, model);
         pump(second, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpSeveral(RTIambassador first, RTIambassador second, CallbackModel model)
         throws Exception {
      for (int pass = 0; pass < 8; ++pass) {
         pump(first, model);
         pump(second, model);
      }
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean joined) {
      if (joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK default-region publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final Set<RegionHandle> regions;
      private final List<RegionDesignatorSnapshot> regionDesignators;

      private Interaction(Object[] arguments, RTIambassador regionInspector) throws Exception {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
         values.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regions = arguments[5] == null ? null
            : new HashSet<>((RegionHandleSet) arguments[5]);
         regionDesignators = arguments[5] == null || regionInspector == null ? null
            : snapshotRegionDesignators(regionInspector, (RegionHandleSet) arguments[5]);
      }
   }

   private static final class RegionDesignatorSnapshot {
      private final Set<String> dimensions;
      private final Map<String, RangeSnapshot> ranges;

      private RegionDesignatorSnapshot(Set<String> dimensions, Map<String, RangeSnapshot> ranges) {
         this.dimensions = dimensions;
         this.ranges = ranges;
      }

      @Override public boolean equals(Object other) {
         if (!(other instanceof RegionDesignatorSnapshot)) return false;
         RegionDesignatorSnapshot that = (RegionDesignatorSnapshot) other;
         return dimensions.equals(that.dimensions) && ranges.equals(that.ranges);
      }

      @Override public int hashCode() { return Objects.hash(dimensions, ranges); }
   }

   private static final class RangeSnapshot {
      private final long lower;
      private final long upper;

      private RangeSnapshot(long lower, long upper) {
         this.lower = lower;
         this.upper = upper;
      }

      @Override public boolean equals(Object other) {
         if (!(other instanceof RangeSnapshot)) return false;
         RangeSnapshot that = (RangeSnapshot) other;
         return lower == that.lower && upper == that.upper;
      }

      @Override public int hashCode() { return Objects.hash(lower, upper); }
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private volatile RTIambassador regionInspector;

      private void inspectRegionDesignatorsWith(RTIambassador ambassador) {
         regionInspector = ambassador;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction(arguments, regionInspector));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK default-region callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
