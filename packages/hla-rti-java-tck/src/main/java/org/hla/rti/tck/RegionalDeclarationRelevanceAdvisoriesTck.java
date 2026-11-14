package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies active/passive regional declarations drive standard relevance advisories. */
final class RegionalDeclarationRelevanceAdvisoriesTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;
   private static final long ADVISORY_OBSERVATION_WINDOW_NANOS = 5_000_000_000L;

   private RegionalDeclarationRelevanceAdvisoriesTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String interactionClassName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied DDM FOM is required");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-regional-declaration-relevance-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean objectPublished = false;
      boolean interactionPublished = false;
      boolean objectSubscribed = false;
      boolean interactionSubscribed = false;
      boolean secondInteractionSubscribed = false;
      RegionHandle objectRegion = null;
      RegionHandle interactionRegion = null;
      RegionHandle secondInteractionRegion = null;
      RegionHandle publisherInteractionRegion = null;
      RegionHandle publisherSecondInteractionRegion = null;
      AttributeSetRegionSetPairList objectPairs = null;
      RegionHandleSet interactionRegions = null;
      RegionHandleSet secondInteractionRegions = null;
      RegionHandleSet publisherInteractionRegions = null;
      RegionHandleSet publisherSecondInteractionRegions = null;
      ObjectClassHandle publisherObjectClass = null;
      ObjectClassHandle subscriberObjectClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandleSet publisherAttributes = null;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle subscriberInteraction = null;
      ParameterHandle publisherParameter = null;
      ParameterHandle subscriberParameter = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         publisher.joinFederationExecution("regional-relevance-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("regional-relevance-subscriber", federation);
         subscriberJoined = true;

         publisherObjectClass = publisher.getObjectClassHandle(objectClassName);
         subscriberObjectClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherObjectClass, attributeName);
         AttributeHandle subscriberAttribute =
            subscriber.getAttributeHandle(subscriberObjectClass, attributeName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         subscriberInteraction = subscriber.getInteractionClassHandle(interactionClassName);
         publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         subscriberParameter = subscriber.getParameterHandle(subscriberInteraction, parameterName);
         DimensionHandle[] publisherDimensions = new DimensionHandle[] {
            publisher.getDimensionHandle(firstDimensionName),
            publisher.getDimensionHandle(secondDimensionName)
         };
         DimensionHandle[] subscriberDimensions = new DimensionHandle[] {
            subscriber.getDimensionHandle(firstDimensionName),
            subscriber.getDimensionHandle(secondDimensionName)
         };
         JavaTckSupport.check(publisherObjectClass.equals(subscriberObjectClass)
               && publisherAttribute.equals(subscriberAttribute)
               && publisherInteraction.equals(subscriberInteraction)
               && publisherParameter.equals(subscriberParameter)
               && !publisherDimensions[0].equals(publisherDimensions[1]),
            "adapter DDM FOM resolved inconsistent class, attribute, interaction, or dimensions");
         checkDimensions(publisher, publisherObjectClass, publisherInteraction, publisherDimensions,
            firstDimensionName, secondDimensionName);
         checkDimensions(subscriber, subscriberObjectClass, subscriberInteraction,
            subscriberDimensions, firstDimensionName, secondDimensionName);

         publisher.setObjectClassRelevanceAdvisorySwitch(true);
         publisher.setInteractionRelevanceAdvisorySwitch(true);
         JavaTckSupport.check(publisher.getObjectClassRelevanceAdvisorySwitch()
               && publisher.getInteractionRelevanceAdvisorySwitch(),
            "regional relevance advisory switches did not enable");
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherObjectClass, publisherAttributes);
         objectPublished = true;
         publisher.publishInteractionClass(publisherInteraction);
         interactionPublished = true;

         objectRegion = createRegion(subscriber, subscriberDimensions);
         objectPairs = objectPairList(subscriber, subscriberAttribute, objectRegion);
         publisherCallbacks.clearObjectAdvisories();
         subscriber.subscribeObjectClassAttributesPassivelyWithRegions(
            subscriberObjectClass, objectPairs);
         objectSubscribed = true;
         settle(publisher, callbackModel);
         JavaTckSupport.check(publisherCallbacks.registrationStarts.isEmpty()
               && publisherCallbacks.registrationStops.isEmpty(),
            "passive regional object subscription changed registration relevance");

         subscriber.subscribeObjectClassAttributesWithRegions(subscriberObjectClass, objectPairs);
         await(publisher, callbackModel,
            () -> !publisherCallbacks.registrationStarts.isEmpty(),
            "regional start-registration advisory");
         JavaTckSupport.check(publisherCallbacks.registrationStarts.size() == 1
               && publisherObjectClass.equals(publisherCallbacks.registrationStarts.get(0)),
            "active regional object subscription returned the wrong advisory");

         subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberObjectClass, objectPairs);
         objectSubscribed = false;
         await(publisher, callbackModel,
            () -> !publisherCallbacks.registrationStops.isEmpty(),
            "regional stop-registration advisory");
         JavaTckSupport.check(publisherCallbacks.registrationStops.size() == 1
               && publisherObjectClass.equals(publisherCallbacks.registrationStops.get(0)),
            "regional object unsubscription returned the wrong advisory");

         publisherCallbacks.clearInteractionAdvisories();
         interactionRegion = createRegion(subscriber, subscriberDimensions);
         interactionRegions = subscriber.getRegionHandleSetFactory().create();
         interactionRegions.add(interactionRegion);
         subscriber.subscribeInteractionClassPassivelyWithRegions(
            subscriberInteraction, interactionRegions);
         interactionSubscribed = true;
         settle(publisher, callbackModel);
         JavaTckSupport.check(publisherCallbacks.interactionsOn.isEmpty()
               && publisherCallbacks.interactionsOff.isEmpty(),
            "passive regional interaction subscription changed interaction relevance");

         subscriber.subscribeInteractionClassWithRegions(subscriberInteraction, interactionRegions);
         await(publisher, callbackModel,
            () -> !publisherCallbacks.interactionsOn.isEmpty(),
            "regional interaction turn-on advisory");
         JavaTckSupport.check(publisherCallbacks.interactionsOn.size() == 1
               && publisherInteraction.equals(publisherCallbacks.interactionsOn.get(0)),
            "active regional interaction subscription returned the wrong advisory");

         secondInteractionRegion = createRegion(subscriber, subscriberDimensions, 3, 5);
         secondInteractionRegions = regionSet(subscriber, secondInteractionRegion);
         RegionHandleSet emptyInteractionRegions = subscriber.getRegionHandleSetFactory().create();
         subscriber.subscribeInteractionClassPassivelyWithRegions(
            subscriberInteraction, emptyInteractionRegions);
         settle(publisher, callbackModel);
         JavaTckSupport.check(publisherCallbacks.interactionsOn.size() == 1
               && publisherCallbacks.interactionsOff.isEmpty(),
            "an empty regional invocation changed the active state of an existing region pair");

         subscriber.subscribeInteractionClassPassivelyWithRegions(
            subscriberInteraction, secondInteractionRegions);
         secondInteractionSubscribed = true;
         settle(publisher, callbackModel);
         JavaTckSupport.check(publisherCallbacks.interactionsOn.size() == 1
               && publisherCallbacks.interactionsOff.isEmpty(),
            "passive status for a second region changed the first region's active advisory state");

         publisherInteractionRegion = createRegion(publisher, publisherDimensions);
         publisherInteractionRegions = regionSet(publisher, publisherInteractionRegion);
         publisherSecondInteractionRegion = createRegion(publisher, publisherDimensions, 3, 5);
         publisherSecondInteractionRegions = regionSet(publisher, publisherSecondInteractionRegion);
         byte[] subscriptionValue = new byte[] {0x72, 0x65, 0x67};
         byte[] subscriptionTag = new byte[] {0x61, 0x64, 0x64};
         verifyRegionalSubscriptionDelivery(publisher, subscriber, subscriberCallbacks,
            callbackModel, publisherInteraction, publisherParameter,
            publisherSecondInteractionRegions, subscriptionValue, subscriptionTag,
            "newly added passive regional interaction subscription delivery");
         verifyRegionalSubscriptionDelivery(publisher, subscriber, subscriberCallbacks,
            callbackModel, publisherInteraction, publisherParameter,
            publisherInteractionRegions, subscriptionValue, subscriptionTag,
            "existing regional interaction subscription delivery after region addition");

         subscriber.subscribeInteractionClassPassivelyWithRegions(
            subscriberInteraction, interactionRegions);
         await(publisher, callbackModel,
            () -> publisherCallbacks.interactionsOff.size() >= 1,
            "active regional interaction subscription changed to passive");
         JavaTckSupport.check(publisherCallbacks.interactionsOff.size() == 1
               && publisherInteraction.equals(publisherCallbacks.interactionsOff.get(0)),
            "passive regional resubscription returned the wrong turn-off advisory");
         subscriber.subscribeInteractionClassWithRegions(subscriberInteraction, interactionRegions);
         await(publisher, callbackModel,
            () -> publisherCallbacks.interactionsOn.size() >= 2,
            "regional interaction relevance re-enabled after passive transition");
         JavaTckSupport.check(publisherCallbacks.interactionsOn.size() == 2
               && publisherInteraction.equals(publisherCallbacks.interactionsOn.get(1)),
            "active regional resubscription after passive transition returned the wrong advisory");
         subscriber.unsubscribeInteractionClassWithRegions(subscriberInteraction, interactionRegions);
         interactionSubscribed = false;
         await(publisher, callbackModel,
            () -> publisherCallbacks.interactionsOff.size() >= 2,
            "regional interaction turn-off advisory");
         JavaTckSupport.check(publisherCallbacks.interactionsOff.size() == 2
               && publisherInteraction.equals(publisherCallbacks.interactionsOff.get(1)),
            "regional interaction unsubscription returned the wrong turn-off advisory");

         subscriber.subscribeInteractionClassWithRegions(
            subscriberInteraction, secondInteractionRegions);
         await(publisher, callbackModel,
            () -> publisherCallbacks.interactionsOn.size() >= 3,
            "second region's passive subscription activated independently");
         JavaTckSupport.check(publisherCallbacks.interactionsOn.size() == 3
               && publisherInteraction.equals(publisherCallbacks.interactionsOn.get(2)),
            "activating the second region returned the wrong interaction advisory");
         subscriber.unsubscribeInteractionClassWithRegions(
            subscriberInteraction, secondInteractionRegions);
         secondInteractionSubscribed = false;
         await(publisher, callbackModel,
            () -> publisherCallbacks.interactionsOff.size() >= 3,
            "second region's active subscription turned off independently");
         JavaTckSupport.check(publisherCallbacks.interactionsOff.size() == 3
               && publisherInteraction.equals(publisherCallbacks.interactionsOff.get(2)),
            "unsubscribing the second region returned the wrong interaction advisory");

         subscriber.subscribeInteractionClassWithRegions(
            subscriberInteraction, secondInteractionRegions);
         secondInteractionSubscribed = true;
         await(publisher, callbackModel,
            () -> publisherCallbacks.interactionsOn.size() >= 4,
            "omitted regional interaction subscription indicator defaulted active");
         JavaTckSupport.check(publisherCallbacks.interactionsOn.size() == 4
               && publisherInteraction.equals(publisherCallbacks.interactionsOn.get(3)),
            "omitting the regional interaction subscription indicator returned the wrong advisory");
         subscriber.unsubscribeInteractionClassWithRegions(
            subscriberInteraction, secondInteractionRegions);
         secondInteractionSubscribed = false;
         await(publisher, callbackModel,
            () -> publisherCallbacks.interactionsOff.size() >= 4,
            "omitted-indicator regional interaction subscription turned off");
         JavaTckSupport.check(publisherCallbacks.interactionsOff.size() == 4
               && publisherInteraction.equals(publisherCallbacks.interactionsOff.get(3)),
            "unsubscribing the omitted-indicator region returned the wrong advisory");

         subscriber.deleteRegion(secondInteractionRegion);
         secondInteractionRegion = null;
         RegionHandleSet deletedInteractionRegions = secondInteractionRegions;
         InteractionClassHandle declaredInteractionClass = subscriberInteraction;
         JavaTckSupport.expectFailure(
            () -> subscriber.subscribeInteractionClassPassivelyWithRegions(
               declaredInteractionClass, deletedInteractionRegions),
            "InvalidRegion", "subscribing with a deleted regional interaction designator");
         subscriber.deleteRegion(interactionRegion);
         interactionRegion = null;
      } finally {
         if (subscriberJoined && secondInteractionSubscribed && subscriberInteraction != null
               && secondInteractionRegions != null) {
            try { subscriber.unsubscribeInteractionClassWithRegions(
               subscriberInteraction, secondInteractionRegions); } catch (Exception ignored) { }
         }
         if (subscriberJoined && interactionSubscribed && subscriberInteraction != null
               && interactionRegions != null) {
            try { subscriber.unsubscribeInteractionClassWithRegions(subscriberInteraction, interactionRegions); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined && secondInteractionRegion != null) {
            try { subscriber.deleteRegion(secondInteractionRegion); } catch (Exception ignored) { }
         }
         if (subscriberJoined && objectSubscribed && subscriberObjectClass != null
               && objectPairs != null) {
            try { subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberObjectClass, objectPairs); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined && interactionRegion != null) {
            try { subscriber.deleteRegion(interactionRegion); } catch (Exception ignored) { }
         }
         if (subscriberJoined && objectRegion != null) {
            try { subscriber.deleteRegion(objectRegion); } catch (Exception ignored) { }
         }
         if (publisherJoined && publisherSecondInteractionRegion != null) {
            try { publisher.deleteRegion(publisherSecondInteractionRegion); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && publisherInteractionRegion != null) {
            try { publisher.deleteRegion(publisherInteractionRegion); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && interactionPublished && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && objectPublished && publisherObjectClass != null
               && publisherAttributes != null) {
            try { publisher.unpublishObjectClassAttributes(publisherObjectClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined) resign(subscriber);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   static void runStartRegistrationSwitchGating(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied DDM FOM is required");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder subscriberCallbacks = new Recorder();
      String federation = "java-tck-start-registration-switch-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean objectPublished = false;
      boolean objectSubscribed = false;
      RegionHandle region = null;
      AttributeSetRegionSetPairList subscription = null;
      ObjectClassHandle publisherObjectClass = null;
      ObjectClassHandle subscriberObjectClass = null;
      AttributeHandleSet publisherAttributes = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("start-registration-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("start-registration-subscriber", federation);
         subscriberJoined = true;

         publisherObjectClass = publisher.getObjectClassHandle(objectClassName);
         subscriberObjectClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherObjectClass, attributeName);
         AttributeHandle subscriberAttribute =
            subscriber.getAttributeHandle(subscriberObjectClass, attributeName);
         DimensionHandle[] publisherDimensions = new DimensionHandle[] {
            publisher.getDimensionHandle(firstDimensionName),
            publisher.getDimensionHandle(secondDimensionName)
         };
         DimensionHandle[] subscriberDimensions = new DimensionHandle[] {
            subscriber.getDimensionHandle(firstDimensionName),
            subscriber.getDimensionHandle(secondDimensionName)
         };
         DimensionHandleSet publisherAvailableDimensions =
            publisher.getAvailableDimensionsForObjectClass(publisherObjectClass);
         DimensionHandleSet subscriberAvailableDimensions =
            subscriber.getAvailableDimensionsForObjectClass(subscriberObjectClass);
         JavaTckSupport.check(publisherObjectClass.equals(subscriberObjectClass)
               && publisherAttribute.equals(subscriberAttribute)
               && !publisherDimensions[0].equals(publisherDimensions[1])
               && publisherAvailableDimensions.contains(publisherDimensions[0])
               && publisherAvailableDimensions.contains(publisherDimensions[1])
               && subscriberAvailableDimensions.contains(subscriberDimensions[0])
               && subscriberAvailableDimensions.contains(subscriberDimensions[1])
               && publisher.getDimensionName(publisherDimensions[0]).equals(firstDimensionName)
               && publisher.getDimensionName(publisherDimensions[1]).equals(secondDimensionName)
               && publisher.getDimensionUpperBound(publisherDimensions[0]) >= 2
               && publisher.getDimensionUpperBound(publisherDimensions[1]) >= 2,
            "adapter DDM FOM omitted the configured object class, attribute, or dimensions");

         publisher.setObjectClassRelevanceAdvisorySwitch(false);
         JavaTckSupport.check(!publisher.getObjectClassRelevanceAdvisorySwitch(),
            "object-class relevance advisory switch did not disable");
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherObjectClass, publisherAttributes);
         objectPublished = true;
         region = createRegion(subscriber, subscriberDimensions);
         subscription = objectPairList(subscriber, subscriberAttribute, region);

         publisherCallbacks.clearObjectAdvisories();
         subscriber.subscribeObjectClassAttributesWithRegions(
            subscriberObjectClass, subscription);
         objectSubscribed = true;
         observeAdvisoryWindow(publisher, callbackModel);
         JavaTckSupport.check(publisherCallbacks.registrationStarts.isEmpty(),
            "Start Registration was invoked while object-class relevance advisories were disabled");

         publisher.setObjectClassRelevanceAdvisorySwitch(true);
         JavaTckSupport.check(publisher.getObjectClassRelevanceAdvisorySwitch(),
            "object-class relevance advisory switch did not enable with the subscription active");
         observeAdvisoryWindow(publisher, callbackModel);
         JavaTckSupport.check(publisherCallbacks.registrationStarts.isEmpty(),
            "Start Registration suppressed while advisories were disabled was replayed after re-enabling");

         publisher.setObjectClassRelevanceAdvisorySwitch(false);
         JavaTckSupport.check(!publisher.getObjectClassRelevanceAdvisorySwitch(),
            "object-class relevance advisory switch did not disable before Stop Registration");
         publisherCallbacks.clearObjectAdvisories();
         subscriber.unsubscribeObjectClassAttributesWithRegions(
            subscriberObjectClass, subscription);
         objectSubscribed = false;
         observeAdvisoryWindow(publisher, callbackModel);
         JavaTckSupport.check(publisherCallbacks.registrationStops.isEmpty(),
            "Stop Registration was invoked while object-class relevance advisories were disabled");

         publisher.setObjectClassRelevanceAdvisorySwitch(true);
         JavaTckSupport.check(publisher.getObjectClassRelevanceAdvisorySwitch(),
            "object-class relevance advisory switch did not enable");
         publisherCallbacks.clearObjectAdvisories();
         subscriber.subscribeObjectClassAttributesWithRegions(
            subscriberObjectClass, subscription);
         objectSubscribed = true;
         await(publisher, callbackModel,
            () -> !publisherCallbacks.registrationStarts.isEmpty(),
            "Start Registration after enabling object-class relevance advisories");
         JavaTckSupport.check(publisherCallbacks.registrationStarts.size() == 1
               && publisherObjectClass.equals(publisherCallbacks.registrationStarts.get(0)),
            "enabled switch did not produce exactly one Start Registration for the published class");

         publisherCallbacks.clearObjectAdvisories();
         subscriber.unsubscribeObjectClassAttributesWithRegions(
            subscriberObjectClass, subscription);
         objectSubscribed = false;
         await(publisher, callbackModel,
            () -> !publisherCallbacks.registrationStops.isEmpty(),
            "Stop Registration after enabling object-class relevance advisories");
         JavaTckSupport.check(publisherCallbacks.registrationStops.size() == 1
               && publisherObjectClass.equals(publisherCallbacks.registrationStops.get(0)),
            "enabled switch did not produce exactly one Stop Registration for the published class");
      } finally {
         if (subscriberJoined && objectSubscribed && subscriberObjectClass != null
               && subscription != null) {
            try { subscriber.unsubscribeObjectClassAttributesWithRegions(
               subscriberObjectClass, subscription); } catch (Exception ignored) { }
         }
         if (subscriberJoined && region != null) {
            try { subscriber.deleteRegion(region); } catch (Exception ignored) { }
         }
         if (publisherJoined && objectPublished && publisherObjectClass != null
               && publisherAttributes != null) {
            try { publisher.unpublishObjectClassAttributes(
               publisherObjectClass, publisherAttributes); } catch (Exception ignored) { }
         }
         if (subscriberJoined) resign(subscriber);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         InteractionClassHandle interaction, DimensionHandle[] dimensions, String firstName,
         String secondName) throws Exception {
      DimensionHandleSet objectDimensions =
         ambassador.getAvailableDimensionsForObjectClass(objectClass);
      DimensionHandleSet interactionDimensions =
         ambassador.getAvailableDimensionsForInteractionClass(interaction);
      JavaTckSupport.check(objectDimensions.contains(dimensions[0])
            && objectDimensions.contains(dimensions[1])
            && interactionDimensions.contains(dimensions[0])
            && interactionDimensions.contains(dimensions[1])
            && ambassador.getDimensionName(dimensions[0]).equals(firstName)
            && ambassador.getDimensionName(dimensions[1]).equals(secondName)
            && ambassador.getDimensionUpperBound(dimensions[0]) >= 4
            && ambassador.getDimensionUpperBound(dimensions[1]) >= 4,
         "regional relevance FOM omitted an adapter-supplied dimension or range");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle[] dimensions)
         throws Exception {
      return createRegion(ambassador, dimensions, 0, 2);
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle[] dimensions,
         long lower, long upper) throws Exception {
      DimensionHandleSet set = ambassador.getDimensionHandleSetFactory().create();
      set.add(dimensions[0]);
      set.add(dimensions[1]);
      RegionHandle region = ambassador.createRegion(set);
      ambassador.setRangeBounds(region, dimensions[0], new RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, dimensions[1], new RangeBounds(lower, upper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      return regions;
   }

   private static void verifyRegionalSubscriptionDelivery(RTIambassador publisher,
         RTIambassador subscriber, Recorder subscriberCallbacks, CallbackModel callbackModel,
         InteractionClassHandle interaction, ParameterHandle parameter,
         RegionHandleSet sourceRegions, byte[] value, byte[] tag, String description)
         throws Exception {
      ParameterHandleValueMap parameters =
         publisher.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value.clone());
      publisher.sendInteractionWithRegions(interaction, parameters, sourceRegions, tag.clone());
      await(subscriber, callbackModel,
         () -> !subscriberCallbacks.receivedInteractions.isEmpty(), description);
      ReceivedInteraction received = subscriberCallbacks.receivedInteractions.get(0);
      JavaTckSupport.check(subscriberCallbacks.receivedInteractions.size() == 1
            && interaction.equals(received.interaction) && received.parameters.size() == 1
            && Arrays.equals(value, received.parameters.get(parameter))
            && Arrays.equals(tag, received.tag),
         description + " returned the wrong interaction data");
      subscriberCallbacks.receivedInteractions.clear();
   }

   private static AttributeSetRegionSetPairList objectPairList(RTIambassador ambassador,
         AttributeHandle attribute, RegionHandle region) throws Exception {
      AttributeHandleSet attributes = JavaTckSupport.attributeSet(ambassador, attribute);
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      pairs.add(new AttributeRegionAssociation(attributes, regions));
      return pairs;
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

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else {
         long deadline = System.nanoTime() + 100_000_000L;
         while (System.nanoTime() < deadline) Thread.yield();
      }
   }

   private static void observeAdvisoryWindow(RTIambassador ambassador,
         CallbackModel callbackModel) throws Exception {
      long deadline = System.nanoTime() + ADVISORY_OBSERVATION_WINDOW_NANOS;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.sleep(1L);
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static final class Recorder {
      private final List<ObjectClassHandle> registrationStarts = new CopyOnWriteArrayList<>();
      private final List<ObjectClassHandle> registrationStops = new CopyOnWriteArrayList<>();
      private final List<InteractionClassHandle> interactionsOn = new CopyOnWriteArrayList<>();
      private final List<InteractionClassHandle> interactionsOff = new CopyOnWriteArrayList<>();
      private final List<ReceivedInteraction> receivedInteractions = new CopyOnWriteArrayList<>();

      private void clearObjectAdvisories() {
         registrationStarts.clear();
         registrationStops.clear();
      }

      private void clearInteractionAdvisories() {
         interactionsOn.clear();
         interactionsOff.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("startRegistrationForObjectClass".equals(name)) {
                  registrationStarts.add((ObjectClassHandle) arguments[0]);
               } else if ("stopRegistrationForObjectClass".equals(name)) {
                  registrationStops.add((ObjectClassHandle) arguments[0]);
               } else if ("turnInteractionsOn".equals(name)) {
                  interactionsOn.add((InteractionClassHandle) arguments[0]);
               } else if ("turnInteractionsOff".equals(name)) {
                  interactionsOff.add((InteractionClassHandle) arguments[0]);
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  receivedInteractions.add(new ReceivedInteraction(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional declaration advisories";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }

   private static final class ReceivedInteraction {
      final InteractionClassHandle interaction;
      final ParameterHandleValueMap parameters;
      final byte[] tag;
      ReceivedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
      }
   }
}
