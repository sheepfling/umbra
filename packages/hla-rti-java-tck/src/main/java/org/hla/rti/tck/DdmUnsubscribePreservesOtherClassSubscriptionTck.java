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
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies that removing one active class subscription preserves a regional subscription. */
final class DdmUnsubscribePreservesOtherClassSubscriptionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long NEGATIVE_OBSERVATION_NANOS = 1_000_000_000L;

   private DdmUnsubscribePreservesOtherClassSubscriptionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String ordinaryObjectClassName, String ordinaryAttributeName,
         String regionalObjectClassName, String regionalAttributeName,
         String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-ddm-unsubscribe-isolation-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean ordinaryPublished = false;
      boolean regionalPublished = false;
      boolean ordinarySubscribed = false;
      boolean regionalSubscribed = false;
      RegionHandle sourceRegion = null;
      RegionHandle subscriptionRegion = null;
      ObjectClassHandle publisherOrdinaryClass = null;
      ObjectClassHandle subscriberOrdinaryClass = null;
      ObjectClassHandle publisherRegionalClass = null;
      ObjectClassHandle subscriberRegionalClass = null;
      AttributeHandleSet publisherOrdinaryAttributes = null;
      AttributeHandleSet subscriberOrdinaryAttributes = null;
      AttributeHandleSet publisherRegionalAttributes = null;
      AttributeHandleSet subscriberRegionalAttributes = null;
      AttributeSetRegionSetPairList sourcePairs = null;
      AttributeSetRegionSetPairList subscriptionPairs = null;
      List<ObjectInstanceHandle> regionalObjects = new ArrayList<>();
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, new String[] {fom}, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-ddm-unsubscribe-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-ddm-unsubscribe-subscriber", federation);
         subscriberJoined = true;

         publisherOrdinaryClass = publisher.getObjectClassHandle(ordinaryObjectClassName);
         subscriberOrdinaryClass = subscriber.getObjectClassHandle(ordinaryObjectClassName);
         publisherRegionalClass = publisher.getObjectClassHandle(regionalObjectClassName);
         subscriberRegionalClass = subscriber.getObjectClassHandle(regionalObjectClassName);
         AttributeHandle publisherOrdinaryAttribute = publisher.getAttributeHandle(
            publisherOrdinaryClass, ordinaryAttributeName);
         AttributeHandle subscriberOrdinaryAttribute = subscriber.getAttributeHandle(
            subscriberOrdinaryClass, ordinaryAttributeName);
         AttributeHandle publisherRegionalAttribute = publisher.getAttributeHandle(
            publisherRegionalClass, regionalAttributeName);
         AttributeHandle subscriberRegionalAttribute = subscriber.getAttributeHandle(
            subscriberRegionalClass, regionalAttributeName);
         JavaTckSupport.check(!publisherOrdinaryClass.equals(publisherRegionalClass)
               && publisherOrdinaryClass.equals(subscriberOrdinaryClass)
               && publisherRegionalClass.equals(subscriberRegionalClass)
               && publisherOrdinaryAttribute.equals(subscriberOrdinaryAttribute)
               && publisherRegionalAttribute.equals(subscriberRegionalAttribute),
            "adapter DDM FOM did not resolve two distinct classes with matching local handles");
         ObjectClassHandle ordinaryCallbackClass = subscriberOrdinaryClass;
         ObjectClassHandle regionalCallbackClass = subscriberRegionalClass;

         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle subscriberFirst = subscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle subscriberSecond = subscriber.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(!publisherFirst.equals(publisherSecond)
               && publisherFirst.equals(subscriberFirst)
               && publisherSecond.equals(subscriberSecond),
            "adapter DDM FOM resolved inconsistent dimension handles");
         checkDimensions(publisher, publisherOrdinaryClass, publisherFirst, publisherSecond);
         checkDimensions(publisher, publisherRegionalClass, publisherFirst, publisherSecond);
         checkDimensions(subscriber, subscriberOrdinaryClass, subscriberFirst, subscriberSecond);
         checkDimensions(subscriber, subscriberRegionalClass, subscriberFirst, subscriberSecond);
         long firstUpper = publisher.getDimensionUpperBound(publisherFirst);
         long secondUpper = publisher.getDimensionUpperBound(publisherSecond);
         JavaTckSupport.check(firstUpper >= 2 && secondUpper >= 2
               && subscriber.getDimensionUpperBound(subscriberFirst) == firstUpper
               && subscriber.getDimensionUpperBound(subscriberSecond) == secondUpper,
            "adapter DDM dimensions did not provide matching positive ranges");

         publisherOrdinaryAttributes = JavaTckSupport.attributeSet(
            publisher, publisherOrdinaryAttribute);
         subscriberOrdinaryAttributes = JavaTckSupport.attributeSet(
            subscriber, subscriberOrdinaryAttribute);
         publisherRegionalAttributes = JavaTckSupport.attributeSet(
            publisher, publisherRegionalAttribute);
         subscriberRegionalAttributes = JavaTckSupport.attributeSet(
            subscriber, subscriberRegionalAttribute);
         publisher.publishObjectClassAttributes(publisherOrdinaryClass, publisherOrdinaryAttributes);
         ordinaryPublished = true;
         publisher.publishObjectClassAttributes(publisherRegionalClass, publisherRegionalAttributes);
         regionalPublished = true;

         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond,
            firstUpper, secondUpper);
         subscriptionRegion = createRegion(subscriber, subscriberFirst, subscriberSecond,
            firstUpper, secondUpper);
         sourcePairs = association(publisher, publisherRegionalAttributes, sourceRegion);
         subscriptionPairs = association(subscriber, subscriberRegionalAttributes,
            subscriptionRegion);

         ObjectInstanceHandle ordinaryObject = publisher.registerObjectInstance(
            publisherOrdinaryClass);
         ObjectInstanceHandle regionalObject = publisher.registerObjectInstanceWithRegions(
            publisherRegionalClass, sourcePairs);
         regionalObjects.add(regionalObject);
         JavaTckSupport.check(ordinaryObject != null && regionalObject != null
               && !ordinaryObject.equals(regionalObject),
            "DDM setup registered an invalid or aliased object instance");

         observeNoAdditionalEvents(subscriber, callbackModel, subscriberRecorder, 0, 0,
            "objects were discovered before an attribute was subscribed");
         subscriber.subscribeObjectClassAttributes(
            subscriberOrdinaryClass, subscriberOrdinaryAttributes);
         ordinarySubscribed = true;
         await(subscriber, callbackModel,
            () -> subscriberRecorder.hasDiscovery(ordinaryObject, ordinaryCallbackClass),
            "ordinary discovery after subscribing to the ordinary class attribute");
         JavaTckSupport.check(!subscriberRecorder.hasDiscovery(regionalObject,
               subscriberRegionalClass),
            "ordinary subscription also discovered the regional object");

         subscriber.subscribeObjectClassAttributesWithRegions(
            subscriberRegionalClass, subscriptionPairs);
         regionalSubscribed = true;
         await(subscriber, callbackModel,
            () -> subscriberRecorder.hasDiscovery(regionalObject, regionalCallbackClass),
            "regional discovery after subscribing to the regional class attribute");
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberRecorder.discoveries.size() == 2
               && subscriberRecorder.hasDiscovery(ordinaryObject, publisherOrdinaryClass)
               && subscriberRecorder.hasDiscovery(regionalObject, publisherRegionalClass),
            "the two active subscriptions did not discover exactly their registered classes");

         byte[] ordinaryInitial = new byte[] {0x11};
         byte[] regionalInitial = new byte[] {0x21};
         update(publisher, ordinaryObject, publisherOrdinaryAttribute, ordinaryInitial);
         update(publisher, regionalObject, publisherRegionalAttribute, regionalInitial);
         await(subscriber, callbackModel,
            () -> subscriberRecorder.hasReflection(ordinaryObject,
                  subscriberOrdinaryAttribute, ordinaryInitial)
               && subscriberRecorder.hasReflection(regionalObject,
                  subscriberRegionalAttribute, regionalInitial),
            "initial reflections for both object classes");
         settle(subscriber, callbackModel);
         JavaTckSupport.check(subscriberRecorder.reflections.size() == 2,
            "initial declarations did not independently deliver both class attributes");

         subscriberRecorder.clearEvents();
         subscriber.unsubscribeObjectClassAttributes(
            subscriberOrdinaryClass, subscriberOrdinaryAttributes);
         ordinarySubscribed = false;
         ObjectInstanceHandle unsubscribedOrdinaryObject = publisher.registerObjectInstance(
            publisherOrdinaryClass);
         JavaTckSupport.check(unsubscribedOrdinaryObject != null,
            "ordinary registration after unsubscription returned a null handle");
         byte[] ordinaryAfterUnsubscribe = new byte[] {0x12};
         byte[] regionalAfterUnsubscribe = new byte[] {0x22};
         update(publisher, ordinaryObject, publisherOrdinaryAttribute, ordinaryAfterUnsubscribe);
         update(publisher, regionalObject, publisherRegionalAttribute, regionalAfterUnsubscribe);
         await(subscriber, callbackModel,
            () -> subscriberRecorder.hasReflection(regionalObject,
                  subscriberRegionalAttribute, regionalAfterUnsubscribe),
            "regional reflection after ordinary class unsubscription");
         observeNoAdditionalEvents(subscriber, callbackModel, subscriberRecorder, 0, 1,
            "ordinary unsubscription changed the regional subscription or leaked an ordinary callback");
         JavaTckSupport.check(subscriberRecorder.reflections.size() == 1
               && subscriberRecorder.hasReflection(regionalObject,
                  subscriberRegionalAttribute, regionalAfterUnsubscribe)
               && !subscriberRecorder.hasDiscovery(unsubscribedOrdinaryObject,
                  subscriberOrdinaryClass),
            "unsubscribing one object class changed another class subscription");

         subscriberRecorder.clearEvents();
         subscriber.unsubscribeObjectClassAttributesWithRegions(
            subscriberRegionalClass, subscriptionPairs);
         regionalSubscribed = false;
         ObjectInstanceHandle unsubscribedRegionalObject = publisher.registerObjectInstanceWithRegions(
            publisherRegionalClass, sourcePairs);
         regionalObjects.add(unsubscribedRegionalObject);
         JavaTckSupport.check(unsubscribedRegionalObject != null,
            "regional registration after unsubscription returned a null handle");
         byte[] regionalAfterRegionalUnsubscribe = new byte[] {0x23};
         update(publisher, regionalObject, publisherRegionalAttribute,
            regionalAfterRegionalUnsubscribe);
         observeNoAdditionalEvents(subscriber, callbackModel, subscriberRecorder, 0, 0,
            "regional discovery or reflection continued after its final attribute subscription was removed");
      } finally {
         if (regionalSubscribed && subscriberJoined && subscriberRegionalClass != null
               && subscriptionPairs != null) {
            try { subscriber.unsubscribeObjectClassAttributesWithRegions(
               subscriberRegionalClass, subscriptionPairs); } catch (Exception ignored) { }
         }
         if (ordinarySubscribed && subscriberJoined && subscriberOrdinaryClass != null
               && subscriberOrdinaryAttributes != null) {
            try { subscriber.unsubscribeObjectClassAttributes(
               subscriberOrdinaryClass, subscriberOrdinaryAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && sourcePairs != null) {
            for (ObjectInstanceHandle regionalObject : regionalObjects) {
               try { publisher.unassociateRegionsForUpdates(regionalObject, sourcePairs); }
               catch (Exception ignored) { }
            }
         }
         if (regionalPublished && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(
               publisherRegionalClass, publisherRegionalAttributes); }
            catch (Exception ignored) { }
         }
         if (ordinaryPublished && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(
               publisherOrdinaryClass, publisherOrdinaryAttributes); }
            catch (Exception ignored) { }
         }
         if (subscriptionRegion != null && subscriberJoined) {
            try { subscriber.deleteRegion(subscriptionRegion); } catch (Exception ignored) { }
         }
         if (sourceRegion != null && publisherJoined) {
            try { publisher.deleteRegion(sourceRegion); } catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void checkDimensions(RTIambassador ambassador, ObjectClassHandle objectClass,
         DimensionHandle first, DimensionHandle second) throws Exception {
      DimensionHandleSet available = ambassador.getAvailableDimensionsForObjectClass(objectClass);
      JavaTckSupport.check(available.contains(first) && available.contains(second),
         "adapter DDM class did not expose both configured dimensions");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long firstUpperBound, long secondUpperBound) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, first, new RangeBounds(0, firstUpperBound - 1));
      ambassador.setRangeBounds(region, second, new RangeBounds(0, secondUpperBound - 1));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static AttributeSetRegionSetPairList association(RTIambassador ambassador,
         AttributeHandleSet attributes, RegionHandle region) throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      pairs.add(new AttributeRegionAssociation(attributes, regions));
      return pairs;
   }

   private static void update(RTIambassador publisher, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value);
      publisher.updateAttributeValues(object, values, new byte[0]);
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.isTrue() && System.nanoTime() < deadline) {
         service(ambassador, model);
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void settle(RTIambassador ambassador, CallbackModel model) throws Exception {
      long deadline = System.nanoTime() + 200_000_000L;
      while (System.nanoTime() < deadline) service(ambassador, model);
   }

   private static void observeNoAdditionalEvents(RTIambassador ambassador, CallbackModel model,
         Recorder recorder, int expectedDiscoveries, int expectedReflections, String message)
         throws Exception {
      long deadline = System.nanoTime() + NEGATIVE_OBSERVATION_NANOS;
      while (System.nanoTime() < deadline) {
         service(ambassador, model);
         JavaTckSupport.check(recorder.discoveries.size() == expectedDiscoveries
               && recorder.reflections.size() == expectedReflections,
            message);
      }
   }

   private static void service(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 0.01);
      } else {
         Thread.sleep(1L);
      }
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  discoveries.add(new Discovery(arguments));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK DDM unsubscribe callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private boolean hasDiscovery(ObjectInstanceHandle object, ObjectClassHandle objectClass) {
         return discoveries.stream().anyMatch(
            discovery -> object.equals(discovery.object)
               && objectClass.equals(discovery.objectClass));
      }

      private boolean hasReflection(ObjectInstanceHandle object, AttributeHandle attribute,
            byte[] value) {
         return reflections.stream().anyMatch(reflection -> {
            byte[] observed = reflection.values.get(attribute);
            return object.equals(reflection.object) && observed != null
               && Arrays.equals(value, observed);
         });
      }

      private void clearEvents() {
         discoveries.clear();
         reflections.clear();
      }
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
      private final AttributeHandleValueMap values;

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
      }
   }
}
