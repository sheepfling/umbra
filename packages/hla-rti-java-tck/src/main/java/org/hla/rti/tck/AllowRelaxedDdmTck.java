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
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Tests the standard Allow Relaxed DDM switch for touching object and interaction regions. */
final class AllowRelaxedDdmTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private AllowRelaxedDdmTck() { }

   static void run(RtiFactory factory, String ddmFom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      runObject(factory, ddmFom, switchesFom, timeImplementation, objectClassName,
         attributeName, firstDimensionName, secondDimensionName, callbackModel, false);
      runObject(factory, ddmFom, switchesFom, timeImplementation, objectClassName,
         attributeName, firstDimensionName, secondDimensionName, callbackModel, true);
      runInteraction(factory, ddmFom, switchesFom, timeImplementation, objectClassName,
         attributeName, interactionClassName, parameterName, firstDimensionName,
         secondDimensionName, callbackModel, false);
      runInteraction(factory, ddmFom, switchesFom, timeImplementation, objectClassName,
         attributeName, interactionClassName, parameterName, firstDimensionName,
         secondDimensionName, callbackModel, true);
   }

   private static void runObject(RtiFactory factory, String ddmFom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         String firstDimensionName, String secondDimensionName, CallbackModel model,
         boolean enabled) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder subscriberEvents = new Recorder();
      String federation = "java-tck-relaxed-ddm-object-" + enabled + "-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false, published = false;
      boolean subscribed = false;
      RegionHandle sourceRegion = null, subscriptionRegion = null;
      ObjectInstanceHandle object = null;
      ObjectClassHandle publisherClass = null, subscriberClass = null;
      AttributeHandle publisherAttribute = null, subscriberAttribute = null;
      AttributeSetRegionSetPairList sourcePairs = null, subscriptionPairs = null;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         publisherConnected = true;
         subscriber.connect(subscriberEvents.proxy(), model);
         subscriberConnected = true;
         publisher.createFederationExecution(federation,
            enabled ? new String[] {ddmFom, switchesFom} : new String[] {ddmFom},
            timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-relaxed-ddm-object-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-relaxed-ddm-object-subscriber", federation);
         subscriberJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, attributeName);
         DimensionHandle[] publisherDimensions = dimensions(publisher, publisherClass,
            firstDimensionName, secondDimensionName);
         DimensionHandle[] subscriberDimensions = dimensions(subscriber, subscriberClass,
            firstDimensionName, secondDimensionName);
         check(publisherClass.equals(subscriberClass)
               && publisherAttribute.equals(subscriberAttribute)
               && Arrays.equals(publisherDimensions, subscriberDimensions),
            "adapter FOM resolved inconsistent object, attribute, or dimension handles");
         check(publisher.getAllowRelaxedDDMSwitch() == enabled
               && subscriber.getAllowRelaxedDDMSwitch() == enabled,
            "Allow Relaxed DDM getter did not match adapter FOM composition");

         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         sourceRegion = createRegion(publisher, publisherDimensions, 0, 2, 0, 2);
         subscriptionRegion = createRegion(subscriber, subscriberDimensions, 2, 3, 2, 3);
         subscriber.setConveyRegionDesignatorSetsSwitch(true);
         sourcePairs = pairList(publisher, publisherAttribute, sourceRegion);
         subscriptionPairs = pairList(subscriber, subscriberAttribute, subscriptionRegion);
         subscriber.subscribeObjectClassAttributesWithRegions(subscriberClass,
            subscriptionPairs, "");
         subscribed = true;
         object = publisher.registerObjectInstanceWithRegions(publisherClass, sourcePairs);
         ObjectInstanceHandle registered = object;
         if (enabled) {
            await(subscriber, model, () -> !subscriberEvents.discoveries.isEmpty(),
               "touching object discovery with Allow Relaxed DDM");
            Discovery discovery = subscriberEvents.discoveries.get(0);
            check(registered.equals(discovery.object) && subscriberClass.equals(discovery.objectClass),
               "touching object discovery returned the wrong identity");
         } else {
            settle(subscriber, model);
            check(subscriberEvents.discoveries.isEmpty(),
               "strict DDM discovered an object whose regions only touch");
         }

         byte[] value = new byte[] {0x41, 0x52, 0x44};
         byte[] tag = new byte[] {0x4F, 0x42};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value);
         publisher.updateAttributeValues(object, values, tag);
         if (enabled) {
            await(subscriber, model, () -> !subscriberEvents.reflections.isEmpty(),
               "touching object reflection with Allow Relaxed DDM");
            Reflection reflection = subscriberEvents.reflections.get(0);
            check(registered.equals(reflection.object) && reflection.values.size() == 1
                  && Arrays.equals(value, reflection.values.get(subscriberAttribute))
                  && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer)
                  && reflection.regions != null && reflection.regions.contains(sourceRegion),
               "relaxed touching-region reflection changed payload or source-region metadata");
         } else {
            settle(subscriber, model);
            check(subscriberEvents.reflections.isEmpty(),
               "strict DDM reflected an update whose regions only touch");
         }
      } finally {
         if (subscriberJoined && subscribed && subscriberClass != null && subscriptionPairs != null) {
            try { subscriber.unsubscribeObjectClassAttributesWithRegions(
               subscriberClass, subscriptionPairs); } catch (Exception ignored) { }
         }
         if (publisherJoined && object != null && sourcePairs != null) {
            try { publisher.unassociateRegionsForUpdates(object, sourcePairs); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherClass != null && publisherAttribute != null) {
            try { publisher.unpublishObjectClassAttributes(publisherClass,
               JavaTckSupport.attributeSet(publisher, publisherAttribute)); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined && subscriptionRegion != null) {
            try { subscriber.deleteRegion(subscriptionRegion); } catch (Exception ignored) { }
         }
         if (publisherJoined && sourceRegion != null) {
            try { publisher.deleteRegion(sourceRegion); } catch (Exception ignored) { }
         }
         cleanup(publisher, subscriber, federation, created, publisherJoined, subscriberJoined,
            publisherConnected, subscriberConnected);
      }
   }

   private static void runInteraction(RtiFactory factory, String ddmFom, String switchesFom,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel model, boolean enabled) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder subscriberEvents = new Recorder();
      String federation = "java-tck-relaxed-ddm-interaction-" + enabled + "-" + UUID.randomUUID();
      boolean publisherConnected = false, subscriberConnected = false, created = false;
      boolean publisherJoined = false, subscriberJoined = false, published = false;
      boolean subscribed = false;
      RegionHandle sourceRegion = null, subscriptionRegion = null;
      ObjectClassHandle publisherObjectClass = null;
      InteractionClassHandle publisherInteraction = null, subscriberInteraction = null;
      ParameterHandle publisherParameter = null, subscriberParameter = null;
      RegionHandleSet sourceRegions = null, subscriptionRegions = null;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         publisherConnected = true;
         subscriber.connect(subscriberEvents.proxy(), model);
         subscriberConnected = true;
         publisher.createFederationExecution(federation,
            enabled ? new String[] {ddmFom, switchesFom} : new String[] {ddmFom},
            timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-relaxed-ddm-interaction-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution(
            "java-tck-relaxed-ddm-interaction-subscriber", federation);
         subscriberJoined = true;

         publisherObjectClass = publisher.getObjectClassHandle(objectClassName);
         DimensionHandle[] publisherDimensions = dimensions(publisher, publisherObjectClass,
            firstDimensionName, secondDimensionName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         subscriberInteraction = subscriber.getInteractionClassHandle(interactionClassName);
         publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         subscriberParameter = subscriber.getParameterHandle(subscriberInteraction, parameterName);
         DimensionHandleSet available = publisher.getAvailableDimensionsForInteractionClass(
            publisherInteraction);
         DimensionHandle subscriberFirst = subscriber.getDimensionHandle(firstDimensionName);
         DimensionHandle subscriberSecond = subscriber.getDimensionHandle(secondDimensionName);
         DimensionHandleSet subscriberAvailable =
            subscriber.getAvailableDimensionsForInteractionClass(subscriberInteraction);
         check(available.contains(publisherDimensions[0])
               && available.contains(publisherDimensions[1])
               && available.size() == 2 && publisherInteraction.equals(subscriberInteraction)
               && publisherParameter.equals(subscriberParameter)
               && publisherDimensions[0].equals(subscriberFirst)
               && publisherDimensions[1].equals(subscriberSecond)
               && subscriberAvailable.contains(subscriberFirst)
               && subscriberAvailable.contains(subscriberSecond)
               && subscriberAvailable.size() == 2,
            "adapter interaction FOM resolved inconsistent handles or DDM dimensions");
         check(publisher.getAllowRelaxedDDMSwitch() == enabled
               && subscriber.getAllowRelaxedDDMSwitch() == enabled,
            "Allow Relaxed DDM getter did not match adapter FOM composition");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherDimensions, 0, 2, 0, 2);
         DimensionHandle[] subscriberDimensions = new DimensionHandle[] {
            subscriber.getDimensionHandle(firstDimensionName),
            subscriber.getDimensionHandle(secondDimensionName)
         };
         subscriptionRegion = createRegion(subscriber, subscriberDimensions, 2, 3, 2, 3);
         sourceRegions = regionSet(publisher, sourceRegion);
         subscriptionRegions = regionSet(subscriber, subscriptionRegion);
         subscriber.setConveyRegionDesignatorSetsSwitch(true);
         subscriber.subscribeInteractionClassWithRegions(subscriberInteraction,
            subscriptionRegions);
         subscribed = true;

         byte[] parameterValue = new byte[] {0x49, 0x4E, 0x54};
         byte[] tag = new byte[] {0x52, 0x58};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, parameterValue);
         publisher.sendInteractionWithRegions(publisherInteraction, parameters, sourceRegions, tag);
         if (enabled) {
            await(subscriber, model, () -> !subscriberEvents.interactions.isEmpty(),
               "touching interaction delivery with Allow Relaxed DDM");
            Interaction interaction = subscriberEvents.interactions.get(0);
            check(subscriberInteraction.equals(interaction.interaction)
                  && interaction.parameters.size() == 1
                  && Arrays.equals(parameterValue, interaction.parameters.get(subscriberParameter))
                  && Arrays.equals(tag, interaction.tag) && producer.equals(interaction.producer)
                  && interaction.regions != null && interaction.regions.contains(sourceRegion),
               "relaxed touching-region interaction changed data or region metadata");
         } else {
            settle(subscriber, model);
            check(subscriberEvents.interactions.isEmpty(),
               "strict DDM delivered an interaction whose regions only touch");
         }
      } finally {
         if (subscriberJoined && subscribed && subscriberInteraction != null
               && subscriptionRegions != null) {
            try { subscriber.unsubscribeInteractionClassWithRegions(
               subscriberInteraction, subscriptionRegions); } catch (Exception ignored) { }
         }
         if (publisherJoined && published && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (subscriberJoined && subscriptionRegion != null) {
            try { subscriber.deleteRegion(subscriptionRegion); } catch (Exception ignored) { }
         }
         if (publisherJoined && sourceRegion != null) {
            try { publisher.deleteRegion(sourceRegion); } catch (Exception ignored) { }
         }
         cleanup(publisher, subscriber, federation, created, publisherJoined, subscriberJoined,
            publisherConnected, subscriberConnected);
      }
   }

   private static DimensionHandle[] dimensions(RTIambassador ambassador,
         ObjectClassHandle objectClass, String firstName, String secondName) throws Exception {
      DimensionHandle first = ambassador.getDimensionHandle(firstName);
      DimensionHandle second = ambassador.getDimensionHandle(secondName);
      DimensionHandleSet available = ambassador.getAvailableDimensionsForObjectClass(objectClass);
      DimensionHandleSet expected = ambassador.getDimensionHandleSetFactory().create();
      expected.add(first);
      expected.add(second);
      check(!first.equals(second) && available.equals(expected)
            && firstName.equals(ambassador.getDimensionName(first))
            && secondName.equals(ambassador.getDimensionName(second))
            && ambassador.getDimensionUpperBound(first) >= 3
            && ambassador.getDimensionUpperBound(second) >= 3,
         "adapter object class did not expose the configured two-dimensional DDM space");
      return new DimensionHandle[] {first, second};
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle[] dimensions,
         long firstLower, long firstUpper, long secondLower, long secondUpper) throws Exception {
      DimensionHandleSet set = ambassador.getDimensionHandleSetFactory().create();
      set.add(dimensions[0]);
      set.add(dimensions[1]);
      RegionHandle region = ambassador.createRegion(set);
      ambassador.setRangeBounds(region, dimensions[0], new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, dimensions[1], new RangeBounds(secondLower, secondUpper));
      commit(ambassador, region);
      return region;
   }

   private static AttributeSetRegionSetPairList pairList(RTIambassador ambassador,
         AttributeHandle attribute, RegionHandle region) throws Exception {
      RegionHandleSet regions = regionSet(ambassador, region);
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      pairs.add(new AttributeRegionAssociation(
         JavaTckSupport.attributeSet(ambassador, attribute), regions));
      return pairs;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      return regions;
   }

   private static void commit(RTIambassador ambassador, RegionHandle region) throws Exception {
      ambassador.commitRegionModifications(regionSet(ambassador, region));
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void settle(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else {
         long deadline = System.nanoTime() + 150_000_000L;
         while (System.nanoTime() < deadline) Thread.yield();
      }
   }

   private static void cleanup(RTIambassador publisher, RTIambassador subscriber,
         String federation, boolean created, boolean publisherJoined, boolean subscriberJoined,
         boolean publisherConnected, boolean subscriberConnected) {
      if (subscriberJoined) try {
         subscriber.resignFederationExecution(ResignAction.NO_ACTION);
      } catch (Exception ignored) { }
      if (publisherJoined) try {
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (Exception ignored) { }
      if (created) try { publisher.destroyFederationExecution(federation); }
         catch (Exception ignored) { }
      if (subscriberConnected) try { subscriber.disconnect(); } catch (Exception ignored) { }
      if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
   }

   private static void check(boolean condition, String message) {
      JavaTckSupport.check(condition, message);
   }

   private static final class Discovery {
      final ObjectInstanceHandle object;
      final ObjectClassHandle objectClass;
      Discovery(Object[] args) {
         object = (ObjectInstanceHandle) args[0];
         objectClass = (ObjectClassHandle) args[1];
      }
   }

   private static final class Reflection {
      final ObjectInstanceHandle object;
      final AttributeHandleValueMap values;
      final byte[] tag;
      final FederateHandle producer;
      final RegionHandleSet regions;
      Reflection(Object[] args) {
         object = (ObjectInstanceHandle) args[0];
         values = ((AttributeHandleValueMap) args[1]).clone();
         tag = ((byte[]) args[2]).clone();
         producer = (FederateHandle) args[4];
         regions = args[5] == null ? null : (RegionHandleSet) ((RegionHandleSet) args[5]).clone();
      }
   }

   private static final class Interaction {
      final InteractionClassHandle interaction;
      final ParameterHandleValueMap parameters;
      final byte[] tag;
      final FederateHandle producer;
      final RegionHandleSet regions;
      Interaction(Object[] args) {
         interaction = (InteractionClassHandle) args[0];
         parameters = ((ParameterHandleValueMap) args[1]).clone();
         tag = ((byte[]) args[2]).clone();
         producer = (FederateHandle) args[4];
         regions = args[5] == null ? null : (RegionHandleSet) ((RegionHandleSet) args[5]).clone();
      }
   }

   private static final class Recorder {
      final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && args != null && args.length == 4) {
                  discoveries.add(new Discovery(args));
               } else if ("reflectAttributeValues".equals(name)
                     && args != null && args.length == 6) {
                  reflections.add(new Reflection(args));
               } else if ("receiveInteraction".equals(name)
                     && args != null && args.length == 6) {
                  interactions.add(new Interaction(args));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK Allow Relaxed DDM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
