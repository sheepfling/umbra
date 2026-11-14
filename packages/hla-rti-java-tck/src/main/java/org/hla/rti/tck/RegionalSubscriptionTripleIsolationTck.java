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
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies that regional subscriptions remain independent by class, region, and attribute. */
final class RegionalSubscriptionTripleIsolationTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private RegionalSubscriptionTripleIsolationTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String firstAttributeName, String secondAttributeName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied multi-attribute DDM FOM is required");

      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      RTIambassador filteredSubscriber = factory.getRtiAmbassador();
      Recorder subscriberCallbacks = new Recorder();
      Recorder filteredCallbacks = new Recorder();
      String federation = "java-tck-regional-subscription-triple-isolation-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false;
      boolean filteredConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false, filteredJoined = false;
      boolean published = false, subscriptionActive = false, filteredSubscriptionActive = false;
      boolean secondSubscriptionActive = false;
      ObjectClassHandle publisherClass = null, subscriberClass = null, filteredClass = null;
      AttributeHandle publisherFirst = null, publisherSecond = null;
      AttributeHandle subscriberFirst = null, subscriberSecond = null;
      AttributeHandle filteredFirst = null, filteredSecond = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet subscriberFirstSet = null, subscriberSecondSet = null;
      AttributeHandleSet filteredFirstSet = null, filteredSecondSet = null;
      AttributeSetRegionSetPairList sourcePair = null, secondSourcePair = null;
      AttributeSetRegionSetPairList subscriptionPair = null;
      AttributeSetRegionSetPairList secondSubscriptionPair = null;
      AttributeSetRegionSetPairList filteredSubscriptionPair = null;
      AttributeSetRegionSetPairList firstTriple = null, secondAttributeTriple = null;
      RegionHandle sourceRegion = null, secondSourceRegion = null;
      RegionHandle subscriptionRegion = null, secondSubscriptionRegion = null;
      RegionHandle filteredFirstRegion = null, filteredSecondRegion = null;
      ObjectInstanceHandle object = null, secondObject = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         filteredSubscriber.connect(filteredCallbacks.proxy(), callbackModel);
         filteredConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "regional-triple-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("regional-triple-subscriber", federation);
         subscriberJoined = true;
         filteredSubscriber.joinFederationExecution(
            "regional-triple-filtered-subscriber", federation);
         filteredJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         filteredClass = filteredSubscriber.getObjectClassHandle(objectClassName);
         publisherFirst = publisher.getAttributeHandle(publisherClass, firstAttributeName);
         publisherSecond = publisher.getAttributeHandle(publisherClass, secondAttributeName);
         subscriberFirst = subscriber.getAttributeHandle(subscriberClass, firstAttributeName);
         subscriberSecond = subscriber.getAttributeHandle(subscriberClass, secondAttributeName);
         filteredFirst = filteredSubscriber.getAttributeHandle(filteredClass, firstAttributeName);
         filteredSecond = filteredSubscriber.getAttributeHandle(filteredClass, secondAttributeName);
         DimensionHandle publisherFirstDimension = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecondDimension = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle subscriberFirstDimension = subscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle subscriberSecondDimension = subscriber.getDimensionHandle(secondDimensionName);
         DimensionHandle filteredFirstDimension =
            filteredSubscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle filteredSecondDimension =
            filteredSubscriber.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherClass.equals(subscriberClass)
               && publisherClass.equals(filteredClass)
               && publisherFirst.equals(subscriberFirst)
               && publisherFirst.equals(filteredFirst)
               && publisherSecond.equals(subscriberSecond)
               && publisherSecond.equals(filteredSecond)
               && !publisherFirst.equals(publisherSecond)
               && publisherFirstDimension.equals(subscriberFirstDimension)
               && publisherFirstDimension.equals(filteredFirstDimension)
               && publisherSecondDimension.equals(subscriberSecondDimension)
               && publisherSecondDimension.equals(filteredSecondDimension)
               && !publisherFirstDimension.equals(publisherSecondDimension),
            "regional subscription triple members resolved inconsistent handles");
         final ObjectClassHandle resolvedSubscriberClass = subscriberClass;
         final AttributeHandle resolvedSubscriberFirst = subscriberFirst;
         final AttributeHandle resolvedSubscriberSecond = subscriberSecond;
         final AttributeHandle resolvedFilteredFirst = filteredFirst;
         final AttributeHandle resolvedFilteredSecond = filteredSecond;
         checkDimensions(publisher, publisherClass, publisherFirstDimension, publisherSecondDimension);
         checkDimensions(subscriber, subscriberClass, subscriberFirstDimension, subscriberSecondDimension);
         checkDimensions(filteredSubscriber, filteredClass,
            filteredFirstDimension, filteredSecondDimension);

         publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherFirst, publisherSecond);
         subscriberFirstSet = JavaTckSupport.attributeSet(subscriber, subscriberFirst);
         subscriberSecondSet = JavaTckSupport.attributeSet(subscriber, subscriberSecond);
         filteredFirstSet = JavaTckSupport.attributeSet(filteredSubscriber, filteredFirst);
         filteredSecondSet = JavaTckSupport.attributeSet(filteredSubscriber, filteredSecond);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;

         sourceRegion = createRegion(publisher, publisherFirstDimension, publisherSecondDimension, 1, 3);
         secondSourceRegion = createRegion(
            publisher, publisherFirstDimension, publisherSecondDimension, 5, 7);
         subscriptionRegion = createRegion(
            subscriber, subscriberFirstDimension, subscriberSecondDimension, 1, 3);
         secondSubscriptionRegion = createRegion(
            subscriber, subscriberFirstDimension, subscriberSecondDimension, 5, 7);
         filteredFirstRegion = createRegion(
            filteredSubscriber, filteredFirstDimension, filteredSecondDimension, 1, 3);
         filteredSecondRegion = createRegion(
            filteredSubscriber, filteredFirstDimension, filteredSecondDimension, 5, 7);
         RegionHandle staleSubscriptionRegion = createRegion(
            subscriber, subscriberFirstDimension, subscriberSecondDimension, 8, 9);
         subscriber.deleteRegion(staleSubscriptionRegion);

         sourcePair = pairList(publisher,
            JavaTckSupport.attributeSet(publisher, publisherFirst), sourceRegion,
            JavaTckSupport.attributeSet(publisher, publisherSecond), sourceRegion);
         secondSourcePair = pairList(publisher,
            JavaTckSupport.attributeSet(publisher, publisherFirst), secondSourceRegion);
         subscriptionPair = pairList(subscriber, subscriberFirstSet, subscriptionRegion,
            subscriberSecondSet, subscriptionRegion);
         secondSubscriptionPair = pairList(subscriber, subscriberFirstSet, secondSubscriptionRegion);
         filteredSubscriptionPair = pairList(filteredSubscriber,
            filteredFirstSet, filteredFirstRegion, filteredSecondSet, filteredSecondRegion);
         firstTriple = pairList(subscriber, subscriberFirstSet, subscriptionRegion);
         secondAttributeTriple = pairList(subscriber, subscriberSecondSet, subscriptionRegion);

         AttributeSetRegionSetPairList invalidRegionPair =
            pairList(subscriber, subscriberFirstSet, staleSubscriptionRegion);
         JavaTckSupport.expectFailure(() -> subscriber.subscribeObjectClassAttributesWithRegions(
               resolvedSubscriberClass, invalidRegionPair), "InvalidRegion",
            "regional subscription with a non-region designator");

         subscriber.subscribeObjectClassAttributesWithRegions(subscriberClass, subscriptionPair);
         subscriptionActive = true;
         subscriber.subscribeObjectClassAttributesWithRegions(subscriberClass, subscriptionPair);
         AttributeSetRegionSetPairList emptyUnsubscribePair = emptyPair(subscriber, subscriberFirstSet);
         subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberClass, emptyUnsubscribePair);
         JavaTckSupport.expectFailure(() -> subscriber.unsubscribeObjectClassAttributesWithRegions(
               resolvedSubscriberClass, invalidRegionPair), "InvalidRegion",
            "regional unsubscription with a non-region designator");
         filteredSubscriber.subscribeObjectClassAttributesWithRegions(
            filteredClass, filteredSubscriptionPair);
         filteredSubscriptionActive = true;

         object = publisher.registerObjectInstanceWithRegions(publisherClass, sourcePair);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "regional subscription triple registration returned an invalid object");
         final ObjectInstanceHandle registered = object;
         awaitBoth(subscriber, filteredSubscriber, callbackModel,
            () -> subscriberCallbacks.hasDiscovery(registered)
               && filteredCallbacks.hasDiscovery(registered),
            "regional subscription triple object discovery");

         byte[] firstInitial = new byte[] {0x51, 0x01};
         byte[] secondInitial = new byte[] {0x52, 0x01};
         byte[] initialTag = new byte[] {0x61, 0x01};
         updateBoth(publisher, object, publisherFirst, publisherSecond,
            firstInitial, secondInitial, initialTag);
         awaitBoth(subscriber, filteredSubscriber, callbackModel,
            () -> hasValue(subscriberCallbacks.reflections, registered, resolvedSubscriberFirst,
                  firstInitial, initialTag, producer)
               && hasValue(subscriberCallbacks.reflections, registered, resolvedSubscriberSecond,
                  secondInitial, initialTag, producer)
               && hasValue(filteredCallbacks.reflections, registered, resolvedFilteredFirst,
                  firstInitial, initialTag, producer),
            "both active regional attribute triples before selective unsubscribe");
         settleBoth(subscriber, filteredSubscriber, callbackModel);

         JavaTckSupport.check(filteredCallbacks.reflections.size() == 1,
            "regional subscription did not project the same-region passel in one callback");
         Reflection filteredPassel = filteredCallbacks.reflections.get(0);
         JavaTckSupport.check(object.equals(filteredPassel.object)
               && Arrays.equals(initialTag, filteredPassel.tag)
               && producer.equals(filteredPassel.producer)
               && filteredPassel.values.size() == 1
               && filteredPassel.values.containsKey(resolvedFilteredFirst)
               && !filteredPassel.values.containsKey(resolvedFilteredSecond)
               && Arrays.equals(firstInitial, filteredPassel.values.get(resolvedFilteredFirst)),
            "regional subscription did not project the passel to the matching attribute only");
         JavaTckSupport.check(subscriberCallbacks.reflections.size() == 1
               && subscriberCallbacks.reflections.get(0).values.size() == 2
               && subscriberCallbacks.reflections.get(0).values.containsKey(subscriberFirst)
               && subscriberCallbacks.reflections.get(0).values.containsKey(subscriberSecond)
               && countValue(subscriberCallbacks.reflections, object, subscriberFirst,
                  firstInitial, initialTag, producer) == 1
               && countValue(subscriberCallbacks.reflections, object, subscriberSecond,
                  secondInitial, initialTag, producer) == 1,
            "repeating an existing regional subscription changed callback delivery");

         subscriber.subscribeObjectClassAttributesWithRegions(subscriberClass, secondSubscriptionPair);
         secondSubscriptionActive = true;
         secondObject = publisher.registerObjectInstanceWithRegions(publisherClass, secondSourcePair);
         JavaTckSupport.check(secondObject != null && secondObject.encodedLength() > 0
               && !secondObject.equals(object),
            "second accumulated-region object registration returned an invalid handle");
         final ObjectInstanceHandle secondRegistered = secondObject;
         await(subscriber, callbackModel, () -> subscriberCallbacks.hasDiscovery(secondRegistered),
            "object discovery through a region added by a later subscription invocation");

         subscriberCallbacks.reflections.clear();
         byte[] secondRegionValue = new byte[] {0x53, 0x01};
         byte[] secondRegionTag = new byte[] {0x63, 0x01};
         updateFirst(publisher, secondObject, publisherFirst, secondRegionValue, secondRegionTag);
         await(subscriber, callbackModel,
            () -> hasValue(subscriberCallbacks.reflections, secondRegistered, resolvedSubscriberFirst,
               secondRegionValue, secondRegionTag, producer),
            "first-attribute delivery through the later-added subscription region");
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.reflections.size() == 1
               && countValue(subscriberCallbacks.reflections, secondObject, subscriberFirst,
                  secondRegionValue, secondRegionTag, producer) == 1,
            "later-added regional subscription did not deliver its value exactly once");

         subscriberCallbacks.reflections.clear();
         byte[] firstRegionAfterAddition = new byte[] {0x51, 0x03};
         byte[] secondAttributeAfterAddition = new byte[] {0x52, 0x03};
         byte[] afterAdditionTag = new byte[] {0x61, 0x03};
         updateBoth(publisher, object, publisherFirst, publisherSecond,
            firstRegionAfterAddition, secondAttributeAfterAddition, afterAdditionTag);
         await(subscriber, callbackModel,
            () -> hasValue(subscriberCallbacks.reflections, registered, resolvedSubscriberFirst,
                  firstRegionAfterAddition, afterAdditionTag, producer)
               && hasValue(subscriberCallbacks.reflections, registered, resolvedSubscriberSecond,
                  secondAttributeAfterAddition, afterAdditionTag, producer),
            "original subscription region remains active after a later region is added");
         settle(subscriber, callbackModel);
         List<Reflection> retainedFirstRegion = subscriberCallbacks.reflections;
         JavaTckSupport.check(countValue(retainedFirstRegion, object, subscriberFirst,
               firstRegionAfterAddition, afterAdditionTag, producer) == 1
               && countValue(retainedFirstRegion, object, subscriberSecond,
                  secondAttributeAfterAddition, afterAdditionTag, producer) == 1
               && !retainedFirstRegion.isEmpty()
               && retainedFirstRegion.stream().allMatch(reflection ->
                  registered.equals(reflection.object) && Arrays.equals(afterAdditionTag, reflection.tag)),
            "adding a second subscription region changed delivery through the original region");

         subscriberCallbacks.reflections.clear();
         subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberClass, firstTriple);
         subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberClass, firstTriple);
         byte[] firstAfterUnsubscribe = new byte[] {0x51, 0x02};
         byte[] secondAfterUnsubscribe = new byte[] {0x52, 0x02};
         byte[] postUnsubscribeTag = new byte[] {0x61, 0x02};
         updateBoth(publisher, object, publisherFirst, publisherSecond,
            firstAfterUnsubscribe, secondAfterUnsubscribe, postUnsubscribeTag);
         await(subscriber, callbackModel, () -> !subscriberCallbacks.reflections.isEmpty(),
            "remaining regional attribute triple after selective unsubscribe");
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.reflections.size() == 1,
            "selective regional unsubscribe produced an absent or duplicate reflection");
         Reflection remaining = subscriberCallbacks.reflections.get(0);
         JavaTckSupport.check(remaining.object.equals(object) && remaining.values.size() == 1
               && Arrays.equals(secondAfterUnsubscribe, remaining.values.get(subscriberSecond))
               && Arrays.equals(postUnsubscribeTag, remaining.tag)
               && producer.equals(remaining.producer)
               && !remaining.values.containsKey(subscriberFirst),
            "unsubscribing one regional triple changed the distinct attribute triple");

         subscriberCallbacks.reflections.clear();
         byte[] secondRegionAfterUnsubscribe = new byte[] {0x53, 0x02};
         byte[] secondRegionAfterUnsubscribeTag = new byte[] {0x63, 0x02};
         updateFirst(publisher, secondObject, publisherFirst,
            secondRegionAfterUnsubscribe, secondRegionAfterUnsubscribeTag);
         await(subscriber, callbackModel,
            () -> hasValue(subscriberCallbacks.reflections, secondRegistered, resolvedSubscriberFirst,
               secondRegionAfterUnsubscribe, secondRegionAfterUnsubscribeTag, producer),
            "later-added region remains subscribed after first-region unsubscribe");
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.reflections.size() == 1
               && countValue(subscriberCallbacks.reflections, secondObject, subscriberFirst,
                  secondRegionAfterUnsubscribe, secondRegionAfterUnsubscribeTag, producer) == 1,
            "unsubscribing the first region removed the later-added second region");

         subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberClass,
            secondSubscriptionPair);
         secondSubscriptionActive = false;
         subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberClass, secondAttributeTriple);
         subscriptionActive = false;
         filteredSubscriber.unsubscribeObjectClassAttributesWithRegions(
            filteredClass, filteredSubscriptionPair);
         filteredSubscriptionActive = false;
         publisher.unassociateRegionsForUpdates(object, sourcePair);
         publisher.unassociateRegionsForUpdates(secondObject, secondSourcePair);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         deleteRegion(publisher, sourceRegion); sourceRegion = null;
         deleteRegion(publisher, secondSourceRegion); secondSourceRegion = null;
         deleteRegion(subscriber, subscriptionRegion); subscriptionRegion = null;
         deleteRegion(subscriber, secondSubscriptionRegion); secondSubscriptionRegion = null;
         deleteRegion(filteredSubscriber, filteredFirstRegion); filteredFirstRegion = null;
         deleteRegion(filteredSubscriber, filteredSecondRegion); filteredSecondRegion = null;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         filteredSubscriber.resignFederationExecution(ResignAction.NO_ACTION);
         filteredJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscriberJoined && subscriptionActive && subscriberClass != null) try {
            if (firstTriple != null) subscriber.unsubscribeObjectClassAttributesWithRegions(
               subscriberClass, firstTriple);
            if (secondAttributeTriple != null) subscriber.unsubscribeObjectClassAttributesWithRegions(
               subscriberClass, secondAttributeTriple);
         } catch (Exception ignored) { }
         if (subscriberJoined && secondSubscriptionActive && subscriberClass != null
               && secondSubscriptionPair != null) try {
            subscriber.unsubscribeObjectClassAttributesWithRegions(subscriberClass,
               secondSubscriptionPair);
         } catch (Exception ignored) { }
         if (filteredJoined && filteredSubscriptionActive && filteredClass != null
               && filteredSubscriptionPair != null) try {
            filteredSubscriber.unsubscribeObjectClassAttributesWithRegions(
               filteredClass, filteredSubscriptionPair);
         } catch (Exception ignored) { }
         if (publisherJoined && object != null && sourcePair != null) try {
            publisher.unassociateRegionsForUpdates(object, sourcePair);
         } catch (Exception ignored) { }
         if (publisherJoined && secondObject != null && secondSourcePair != null) try {
            publisher.unassociateRegionsForUpdates(secondObject, secondSourcePair);
         } catch (Exception ignored) { }
         if (publisherJoined && published && publisherClass != null && publisherAttributes != null) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         if (publisherJoined && sourceRegion != null) deleteRegion(publisher, sourceRegion);
         if (publisherJoined && secondSourceRegion != null) deleteRegion(publisher, secondSourceRegion);
         if (subscriberJoined && subscriptionRegion != null) deleteRegion(subscriber, subscriptionRegion);
         if (subscriberJoined && secondSubscriptionRegion != null) {
            deleteRegion(subscriber, secondSubscriptionRegion);
         }
         if (filteredJoined && filteredFirstRegion != null) deleteRegion(filteredSubscriber, filteredFirstRegion);
         if (filteredJoined && filteredSecondRegion != null) deleteRegion(filteredSubscriber, filteredSecondRegion);
         if (subscriberJoined) try {
            subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (filteredJoined) try {
            filteredSubscriber.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (filteredConnected) try { filteredSubscriber.disconnect(); } catch (Exception ignored) { }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      JavaTckSupport.check(!first.equals(second)
            && ambassador.getAvailableDimensionsForObjectClass(objectClass).contains(first)
            && ambassador.getAvailableDimensionsForObjectClass(objectClass).contains(second)
            && ambassador.getDimensionUpperBound(first) >= 7
            && ambassador.getDimensionUpperBound(second) >= 7,
         "multi-attribute DDM object class must expose both dimensions through range 7");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new RangeBounds(lower, upper));
      commitRegion(ambassador, region);
      return region;
   }

   private static AttributeSetRegionSetPairList pairList(RTIambassador ambassador,
         AttributeHandleSet firstAttributes, RegionHandle firstRegion,
         AttributeHandleSet secondAttributes, RegionHandle secondRegion) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(2);
      result.add(new AttributeRegionAssociation(firstAttributes, regionSet(ambassador, firstRegion)));
      result.add(new AttributeRegionAssociation(secondAttributes, regionSet(ambassador, secondRegion)));
      return result;
   }

   private static AttributeSetRegionSetPairList pairList(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes, regionSet(ambassador, region)));
      return result;
   }

   private static AttributeSetRegionSetPairList emptyPair(RTIambassador ambassador,
         AttributeHandleSet attributes) throws Exception {
      AttributeSetRegionSetPairList result =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      result.add(new AttributeRegionAssociation(attributes,
         ambassador.getRegionHandleSetFactory().create()));
      return result;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static void commitRegion(RTIambassador ambassador, RegionHandle region) throws Exception {
      ambassador.commitRegionModifications(regionSet(ambassador, region));
   }

   private static void updateBoth(RTIambassador ambassador, ObjectInstanceHandle object,
         AttributeHandle first, AttributeHandle second, byte[] firstValue,
         byte[] secondValue, byte[] tag) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(2);
      values.put(first, firstValue.clone());
      values.put(second, secondValue.clone());
      ambassador.updateAttributeValues(object, values, tag.clone());
   }

   private static void updateFirst(RTIambassador ambassador, ObjectInstanceHandle object,
         AttributeHandle first, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(first, value.clone());
      ambassador.updateAttributeValues(object, values, tag.clone());
   }

   private static boolean hasValue(List<Reflection> reflections, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag, FederateHandle producer) {
      return countValue(reflections, object, attribute, value, tag, producer) > 0;
   }

   private static int countValue(List<Reflection> reflections, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag, FederateHandle producer) {
      int count = 0;
      for (Reflection reflection : reflections) {
         if (reflection.object.equals(object) && reflection.values.containsKey(attribute)
               && Arrays.equals(value, reflection.values.get(attribute))
               && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer)) {
            ++count;
         }
      }
      return count;
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel callbackModel, java.util.function.BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         pump(first, callbackModel);
         pump(second, callbackModel);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         pump(ambassador, callbackModel);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
      else Thread.sleep(1L);
   }

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(ambassador);
      } else {
         long deadline = System.nanoTime() + 100_000_000L;
         while (System.nanoTime() < deadline) Thread.yield();
      }
   }

   private static void settleBoth(RTIambassador first, RTIambassador second,
         CallbackModel callbackModel) throws Exception {
      settle(first, callbackModel);
      settle(second, callbackModel);
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region) {
      try { ambassador.deleteRegion(region); } catch (Exception ignored) { }
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK no-op federate";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (args == null ? null : args[0]);
            }
            return null;
         });
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private Discovery(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         objectClass = (ObjectClassHandle) arguments[1];
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();
      private final byte[] tag;
      private final FederateHandle producer;
      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap received = (AttributeHandleValueMap) arguments[1];
         received.forEach((handle, value) -> values.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) discoveries.add(new Discovery(arguments));
               else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) reflections.add(new Reflection(arguments));
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional subscription callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
      private boolean hasDiscovery(ObjectInstanceHandle object) {
         return discoveries.stream().anyMatch(discovery -> discovery.object.equals(object));
      }
   }
}
