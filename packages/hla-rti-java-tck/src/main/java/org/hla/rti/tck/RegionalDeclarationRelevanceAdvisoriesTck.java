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
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies active/passive regional declarations drive standard relevance advisories. */
final class RegionalDeclarationRelevanceAdvisoriesTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private RegionalDeclarationRelevanceAdvisoriesTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String interactionClassName, String firstDimensionName, String secondDimensionName,
         CallbackModel callbackModel) throws Exception {
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
      RegionHandle objectRegion = null;
      RegionHandle interactionRegion = null;
      AttributeSetRegionSetPairList objectPairs = null;
      RegionHandleSet interactionRegions = null;
      ObjectClassHandle publisherObjectClass = null;
      ObjectClassHandle subscriberObjectClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandleSet publisherAttributes = null;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle subscriberInteraction = null;
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

         subscriber.unsubscribeInteractionClassWithRegions(subscriberInteraction, interactionRegions);
         interactionSubscribed = false;
         await(publisher, callbackModel,
            () -> !publisherCallbacks.interactionsOff.isEmpty(),
            "regional interaction turn-off advisory");
         JavaTckSupport.check(publisherCallbacks.interactionsOff.size() == 1
               && publisherInteraction.equals(publisherCallbacks.interactionsOff.get(0)),
            "regional interaction unsubscription returned the wrong advisory");
      } finally {
         if (subscriberJoined && interactionSubscribed && subscriberInteraction != null
               && interactionRegions != null) {
            try { subscriber.unsubscribeInteractionClassWithRegions(subscriberInteraction, interactionRegions); }
            catch (Exception ignored) { }
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
      DimensionHandleSet set = ambassador.getDimensionHandleSetFactory().create();
      set.add(dimensions[0]);
      set.add(dimensions[1]);
      RegionHandle region = ambassador.createRegion(set);
      ambassador.setRangeBounds(region, dimensions[0], new RangeBounds(0, 2));
      ambassador.setRangeBounds(region, dimensions[1], new RangeBounds(0, 2));
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      ambassador.commitRegionModifications(regions);
      return region;
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

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static final class Recorder {
      private final List<ObjectClassHandle> registrationStarts = new CopyOnWriteArrayList<>();
      private final List<ObjectClassHandle> registrationStops = new CopyOnWriteArrayList<>();
      private final List<InteractionClassHandle> interactionsOn = new CopyOnWriteArrayList<>();
      private final List<InteractionClassHandle> interactionsOff = new CopyOnWriteArrayList<>();

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
}
