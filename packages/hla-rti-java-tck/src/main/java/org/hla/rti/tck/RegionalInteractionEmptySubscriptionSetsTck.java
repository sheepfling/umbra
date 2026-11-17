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
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Treats empty regional interaction subscription sets as no-ops. */
final class RegionalInteractionEmptySubscriptionSetsTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final byte[] VALUE = new byte[] {0x52};

   private RegionalInteractionEmptySubscriptionSetsTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String interactionName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder subscriberCallbacks = new Recorder(subscriber);
      String federation = "java-tck-regional-interaction-empty-subscriptions-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false, published = false;
      boolean subscribed = false;
      InteractionClassHandle publisherInteraction = null, subscriberInteraction = null;
      ObjectClassHandle publisherObjectClass = null, subscriberObjectClass = null;
      RegionHandle sourceRegion = null, subscriptionRegion = null, absentSubscriptionRegion = null;
      RegionHandleSet sourceRegions = null, subscriptionRegions = null;
      RegionHandleSet absentSubscriptionRegions = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberCallbacks.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom);
         created = true;
         FederateHandle producingFederate = publisher.joinFederationExecution(
            "java-tck-regional-empty-set-publisher", federation);
         publisherJoined = true;
         FederateHandle subscribingFederate = subscriber.joinFederationExecution(
            "java-tck-regional-empty-set-subscriber", federation);
         subscriberJoined = true;
         JavaTckSupport.check(producingFederate != null && producingFederate.encodedLength() > 0
               && subscribingFederate != null && subscribingFederate.encodedLength() > 0,
            "regional interaction subscription-set join returned an invalid federate handle");

         publisherObjectClass = publisher.getObjectClassHandle(objectClassName);
         subscriberObjectClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherObjectClass, attributeName);
         AttributeHandle subscriberAttribute =
            subscriber.getAttributeHandle(subscriberObjectClass, attributeName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         subscriberInteraction = subscriber.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle subscriberParameter =
            subscriber.getParameterHandle(subscriberInteraction, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle subscriberFirst = subscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle subscriberSecond = subscriber.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherObjectClass.encodedLength() > 0
               && subscriberObjectClass.encodedLength() > 0
               && publisherAttribute.encodedLength() > 0
               && subscriberAttribute.encodedLength() > 0
               && publisherInteraction.encodedLength() > 0
               && subscriberInteraction.encodedLength() > 0
               && publisherParameter.encodedLength() > 0
               && subscriberParameter.encodedLength() > 0
               && publisherFirst.encodedLength() > 0
               && publisherSecond.encodedLength() > 0
               && subscriberFirst.encodedLength() > 0
               && subscriberSecond.encodedLength() > 0
               && publisherInteraction.equals(subscriberInteraction)
               && publisherParameter.equals(subscriberParameter)
               && !publisherFirst.equals(publisherSecond),
            "adapter DDM FOM returned inconsistent interaction, parameter, or dimension handles");
         DimensionHandleSet objectDimensions =
            publisher.getAvailableDimensionsForObjectClass(publisherObjectClass);
         JavaTckSupport.check(objectDimensions.contains(publisherFirst)
               && objectDimensions.contains(publisherSecond),
            "adapter object class omitted an adapter-supplied DDM dimension");
         checkDimensions(publisher, publisherInteraction, publisherFirst, publisherSecond,
            firstDimensionName, secondDimensionName);
         checkDimensions(subscriber, subscriberInteraction, subscriberFirst, subscriberSecond,
            firstDimensionName, secondDimensionName);
         subscriberCallbacks.setDimensions(subscriberFirst, subscriberSecond);

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 0, 2);
         subscriptionRegion = createRegion(
            subscriber, subscriberFirst, subscriberSecond, 0, 2);
         absentSubscriptionRegion = createRegion(
            subscriber, subscriberFirst, subscriberSecond, 5, 7);
         sourceRegions = regionSet(publisher, sourceRegion);
         subscriptionRegions = regionSet(subscriber, subscriptionRegion);
         absentSubscriptionRegions = regionSet(subscriber, absentSubscriptionRegion);
         subscriber.setConveyRegionDesignatorSetsSwitch(true);
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, VALUE.clone());

         RegionHandleSet emptyRegions = subscriber.getRegionHandleSetFactory().create();
         subscriber.subscribeInteractionClassWithRegions(subscriberInteraction, emptyRegions);
         send(publisher, publisherInteraction, parameters, sourceRegions, new byte[] {0x45, 0x53, 0x55});
         pumpSeveral(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.interactions.isEmpty(),
            "empty regional interaction subscription created an eligible route");

         subscriber.subscribeInteractionClassWithRegions(subscriberInteraction, subscriptionRegions);
         subscribed = true;
         byte[] subscribedTag = new byte[] {0x53, 0x55, 0x42};
         send(publisher, publisherInteraction, parameters, sourceRegions, subscribedTag);
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.interactions.size() >= 1,
            "delivery after non-empty regional interaction subscription");
         JavaTckSupport.check(subscriberCallbacks.interactions.size() == 1,
            "non-empty regional subscription delivered a duplicate interaction callback");
         assertInteraction(subscriber, subscriberCallbacks.interactions.get(0),
            subscriberInteraction, subscriberParameter, producingFederate,
            subscriberFirst, subscriberSecond, subscribedTag);

         subscriber.unsubscribeInteractionClassWithRegions(subscriberInteraction, emptyRegions);
         byte[] emptyUnsubscribeTag = new byte[] {0x45, 0x55, 0x4E};
         send(publisher, publisherInteraction, parameters, sourceRegions, emptyUnsubscribeTag);
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.interactions.size() >= 2,
            "delivery after empty regional interaction unsubscription");
         JavaTckSupport.check(subscriberCallbacks.interactions.size() == 2,
            "empty regional unsubscription changed the active route");
         assertInteraction(subscriber, subscriberCallbacks.interactions.get(1),
            subscriberInteraction, subscriberParameter, producingFederate,
            subscriberFirst, subscriberSecond, emptyUnsubscribeTag);

         subscriber.unsubscribeInteractionClassWithRegions(
            subscriberInteraction, absentSubscriptionRegions);
         byte[] absentUnsubscribeTag = new byte[] {0x41, 0x42, 0x53};
         send(publisher, publisherInteraction, parameters, sourceRegions, absentUnsubscribeTag);
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.interactions.size() >= 3,
            "delivery after unsubscribing an absent regional interaction region");
         JavaTckSupport.check(subscriberCallbacks.interactions.size() == 3,
            "unsubscribing an absent region changed the active route");
         assertInteraction(subscriber, subscriberCallbacks.interactions.get(2),
            subscriberInteraction, subscriberParameter, producingFederate,
            subscriberFirst, subscriberSecond, absentUnsubscribeTag);

         subscriber.deleteRegion(absentSubscriptionRegion);
         absentSubscriptionRegion = null;
         InteractionClassHandle deletedRegionInteraction = subscriberInteraction;
         RegionHandleSet deletedRegionSet = absentSubscriptionRegions;
         JavaTckSupport.expectFailure(() -> subscriber.unsubscribeInteractionClassWithRegions(
               deletedRegionInteraction, deletedRegionSet),
            "InvalidRegion", "regional interaction unsubscription with a deleted region");
         byte[] deletedRegionUnsubscribeTag = new byte[] {0x44, 0x45, 0x4C};
         send(publisher, publisherInteraction, parameters, sourceRegions, deletedRegionUnsubscribeTag);
         await(subscriber, callbackModel,
            () -> subscriberCallbacks.interactions.size() >= 4,
            "delivery after rejecting a deleted-region unsubscription");
         JavaTckSupport.check(subscriberCallbacks.interactions.size() == 4,
            "rejected deleted-region unsubscription changed the active route");
         assertInteraction(subscriber, subscriberCallbacks.interactions.get(3),
            subscriberInteraction, subscriberParameter, producingFederate,
            subscriberFirst, subscriberSecond, deletedRegionUnsubscribeTag);

         subscriber.unsubscribeInteractionClassWithRegions(
            subscriberInteraction, subscriptionRegions);
         subscribed = false;
         byte[] unsubscribedTag = new byte[] {0x55, 0x4E, 0x53};
         send(publisher, publisherInteraction, parameters, sourceRegions, unsubscribedTag);
         pumpSeveral(subscriber, callbackModel);
         JavaTckSupport.check(subscriberCallbacks.interactions.size() == 4,
            "non-empty regional unsubscription failed to remove the route");

         subscriber.deleteRegion(subscriptionRegion);
         subscriptionRegion = null;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
         subscriberJoined = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && subscriberJoined && subscriptionRegions != null) try {
            subscriber.unsubscribeInteractionClassWithRegions(
               subscriberInteraction, subscriptionRegions);
         } catch (Exception ignored) { }
         deleteRegion(subscriber, subscriptionRegion, subscriberJoined);
         deleteRegion(subscriber, absentSubscriptionRegion, subscriberJoined);
         deleteRegion(publisher, sourceRegion, publisherJoined);
         if (published && publisherJoined) try {
            publisher.unpublishInteractionClass(publisherInteraction);
         } catch (Exception ignored) { }
         if (subscriberJoined) try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void send(RTIambassador publisher, InteractionClassHandle interaction,
         ParameterHandleValueMap parameters, RegionHandleSet regions, byte[] tag) throws Exception {
      publisher.sendInteractionWithRegions(interaction, parameters, regions, tag.clone());
   }

   private static void checkDimensions(RTIambassador ambassador,
         InteractionClassHandle interaction, DimensionHandle first, DimensionHandle second,
         String firstName, String secondName)
         throws Exception {
      java.util.Set<DimensionHandle> dimensions =
         ambassador.getAvailableDimensionsForInteractionClass(interaction);
      JavaTckSupport.check(dimensions.contains(first) && dimensions.contains(second)
            && firstName.equals(ambassador.getDimensionName(first))
            && secondName.equals(ambassador.getDimensionName(second))
            && !first.equals(second)
            && ambassador.getDimensionUpperBound(first) >= 4
            && ambassador.getDimensionUpperBound(second) >= 4,
         "adapter interaction class does not expose the configured DDM dimensions and bounds");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
      JavaTckSupport.check(region != null && region.encodedLength() > 0,
         "empty-subscription fixture returned an invalid region handle");
      ambassador.setRangeBounds(region, first, new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.setRangeBounds(region, second, new hla.rti1516_2025.RangeBounds(lower, upper));
      RegionHandleSet regions = regionSet(ambassador, region);
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      result.add(region);
      return result;
   }

   private static void assertInteraction(RTIambassador receiver, Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, DimensionHandle expectedFirstDimension,
         DimensionHandle expectedSecondDimension, byte[] expectedTag)
         throws Exception {
      JavaTckSupport.check(expectedClass.equals(interaction.interaction)
            && interaction.parameters.size() == 1
            && Arrays.equals(VALUE, interaction.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, interaction.tag)
            && expectedProducer.equals(interaction.producer),
         "regional interaction changed its class, parameter, value, tag, or producer");
      TransportationTypeHandle transportation = interaction.transportation;
      JavaTckSupport.check(transportation != null
            && transportation.encodedLength() > 0
            && !receiver.getTransportationTypeName(transportation).isEmpty(),
         "regional interaction returned an unresolvable transportation type");
      JavaTckSupport.check(interaction.regions != null && interaction.regions.size() == 1,
         "regional interaction omitted its source-region designator");
      RegionHandle callbackRegion = interaction.regions.iterator().next();
      JavaTckSupport.check(callbackRegion != null && callbackRegion.encodedLength() > 0,
         "regional interaction callback supplied an invalid receiver region copy");
      JavaTckSupport.check(interaction.regionError == null
            && interaction.regionDimensions != null && interaction.regionDimensions.size() == 2
            && interaction.regionDimensions.contains(expectedFirstDimension)
            && interaction.regionDimensions.contains(expectedSecondDimension),
         "regional interaction callback region copy changed its dimensions");
      RangeBounds firstBounds = interaction.firstBounds;
      RangeBounds secondBounds = interaction.secondBounds;
      JavaTckSupport.check(firstBounds.lower == 0 && firstBounds.upper == 2
            && secondBounds.lower == 0 && secondBounds.upper == 2,
         "regional interaction callback region copy changed its range bounds");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(ambassador, model);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpSeveral(RTIambassador ambassador, CallbackModel model) throws Exception {
      for (int pass = 0; pass < 8; ++pass) pump(ambassador, model);
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
               if ("toString".equals(method.getName())) return "Java TCK regional interaction publisher";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private Set<RegionHandle> regions;
      private Set<DimensionHandle> regionDimensions;
      private RangeBounds firstBounds;
      private RangeBounds secondBounds;
      private String regionError;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final RTIambassador receiver;
      private DimensionHandle firstDimension;
      private DimensionHandle secondDimension;

      private Recorder(RTIambassador receiver) {
         this.receiver = receiver;
      }

      private void setDimensions(DimensionHandle first, DimensionHandle second) {
         firstDimension = first;
         secondDimension = second;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  Interaction interaction = new Interaction();
                  interaction.interaction = (InteractionClassHandle) arguments[0];
                  interaction.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  interaction.tag = ((byte[]) arguments[2]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[3];
                  interaction.producer = (FederateHandle) arguments[4];
                  if (arguments[5] != null) {
                     interaction.regions = new HashSet<>((RegionHandleSet) arguments[5]);
                     try {
                        if (interaction.regions.size() == 1) {
                           RegionHandle callbackRegion = interaction.regions.iterator().next();
                           interaction.regionDimensions =
                              new HashSet<>(receiver.getDimensionHandleSet(callbackRegion));
                           interaction.firstBounds =
                              receiver.getRangeBounds(callbackRegion, firstDimension);
                           interaction.secondBounds =
                              receiver.getRangeBounds(callbackRegion, secondDimension);
                        }
                     } catch (Exception failure) {
                        interaction.regionError = failure.toString();
                     }
                  }
                  interactions.add(interaction);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regional interaction callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
