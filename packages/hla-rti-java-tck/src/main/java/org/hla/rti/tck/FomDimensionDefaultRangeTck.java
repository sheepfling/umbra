package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
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
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Public FOM dimension and region default-range boundary checks. */
final class FomDimensionDefaultRangeTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private FomDimensionDefaultRangeTck() { }

   static void run(RtiFactory factory, String outside, String bounded,
         String mim, String timeImplementation, CallbackModel callbackModel)
         throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      RTIambassador endpoint = factory.getRtiAmbassador();
      Recorder memberCallbacks = new Recorder();
      Recorder endpointCallbacks = new Recorder();
      String federation = "java-tck-dimension-default-range-" + UUID.randomUUID();
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean endpointConnected = false;
      boolean creatorJoined = false;
      boolean memberJoined = false;
      boolean endpointJoined = false;
      boolean created = false;
      RegionHandle region = null;
      FederateHandle creatorFederate = null;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(memberCallbacks.proxy(), callbackModel);
         memberConnected = true;
         endpoint.connect(endpointCallbacks.proxy(), callbackModel);
         endpointConnected = true;
         JavaTckSupport.expectFailure(() -> creator.createFederationExecutionWithMIM(
               federation, new String[] {outside}, mim, timeImplementation),
            "InconsistentFOM", "dimension default range beyond upperBound");

         creator.createFederationExecutionWithMIM(federation,
            new String[] {bounded}, mim, timeImplementation);
         created = true;
         creatorFederate = creator.joinFederationExecution(
            "java-tck-dimension-range-creator", federation);
         creatorJoined = true;
         member.joinFederationExecution("java-tck-dimension-range-member", federation);
         memberJoined = true;
         endpoint.joinFederationExecution("java-tck-dimension-range-endpoint", federation);
         endpointJoined = true;
         String name = "TckBoundedDefaultDimension";
         DimensionHandle dimension = member.getDimensionHandle(name);
         JavaTckSupport.check(dimension != null
               && name.equals(member.getDimensionName(dimension)),
            "bounded dimension default range did not compose publicly");
         long upperBound = member.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound == 100,
            "bounded dimension did not expose its exact upper bound");
         DimensionHandleSet dimensions =
            member.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         region = member.createRegion(dimensions);
         JavaTckSupport.check(region != null && region.encodedLength() > 0,
            "bounded default dimension could not be used in a region");
         RangeBounds initialBounds = member.getRangeBounds(region, dimension);
         JavaTckSupport.check(initialBounds.lower == 0
               && initialBounds.upper == upperBound,
            "a new region did not receive the FOM-supplied default range");
         RegionHandleSet regions = member.getRegionHandleSetFactory().create();
         regions.add(region);
         member.commitRegionModifications(regions);
         RangeBounds committedBounds = member.getRangeBounds(region, dimension);
         JavaTckSupport.check(committedBounds.lower == 0
               && committedBounds.upper == upperBound,
            "committing a default-only region changed the FOM-supplied range");
         member.setRangeBounds(region, dimension, new RangeBounds(0, 1));
         RangeBounds minimumSpan = member.getRangeBounds(region, dimension);
         JavaTckSupport.check(minimumSpan.lower == 0 && minimumSpan.upper == 1,
            "the minimum one-unit region interval did not round-trip");
         RegionHandle rangeTestRegion = region;
         JavaTckSupport.expectFailure(
            () -> member.setRangeBounds(rangeTestRegion, dimension, new RangeBounds(1, 1)),
            "InvalidRangeBound", "a zero-width region interval");
         member.setRangeBounds(region, dimension, new RangeBounds(0, upperBound));
         RangeBounds explicitBounds = member.getRangeBounds(region, dimension);
         JavaTckSupport.check(explicitBounds.lower == 0
               && explicitBounds.upper == upperBound,
            "an explicit full-dimension interval did not preserve bound order");
         member.commitRegionModifications(regions);
         RangeBounds recommittedBounds = member.getRangeBounds(region, dimension);
         JavaTckSupport.check(recommittedBounds.lower == 0
               && recommittedBounds.upper == upperBound,
            "committing explicit full-dimension bounds changed the interval");
         member.deleteRegion(region);
         region = null;
         verifyObjectRegionRouting(creator, member, endpoint, memberCallbacks,
            endpointCallbacks, dimension, creatorFederate, callbackModel);
         verifyInteractionRegionRouting(creator, member, endpoint, memberCallbacks,
            endpointCallbacks, dimension, upperBound, creatorFederate, callbackModel);
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         endpoint.resignFederationExecution(ResignAction.NO_ACTION);
         endpointJoined = false;
         creator.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         creatorJoined = false;
         creator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (region != null) try { member.deleteRegion(region); }
            catch (Exception ignored) { }
         if (memberJoined) try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (endpointJoined) try { endpoint.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (creatorJoined) try {
            creator.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (endpointConnected) try { endpoint.disconnect(); }
            catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); }
            catch (Exception ignored) { }
         if (creatorConnected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static void verifyObjectRegionRouting(RTIambassador creator, RTIambassador member,
         RTIambassador endpoint, Recorder memberCallbacks, Recorder endpointCallbacks,
         DimensionHandle sourceDimension, FederateHandle creatorFederate,
         CallbackModel callbackModel) throws Exception {
      String objectName = "HLAobjectRoot.TckBoundedDefaultObject";
      ObjectClassHandle creatorClass = creator.getObjectClassHandle(objectName);
      ObjectClassHandle memberClass = member.getObjectClassHandle(objectName);
      ObjectClassHandle endpointClass = endpoint.getObjectClassHandle(objectName);
      AttributeHandle creatorAttribute = creator.getAttributeHandle(creatorClass, "Value");
      AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, "Value");
      AttributeHandle endpointAttribute = endpoint.getAttributeHandle(endpointClass, "Value");
      DimensionHandle memberDimension = member.getDimensionHandle("TckBoundedDefaultDimension");
      DimensionHandle endpointDimension = endpoint.getDimensionHandle("TckBoundedDefaultDimension");
      JavaTckSupport.check(creatorClass != null && memberClass != null && endpointClass != null
            && creatorAttribute != null && memberAttribute != null && endpointAttribute != null
            && creatorAttribute.equals(memberAttribute) && creatorAttribute.equals(endpointAttribute)
            && creator.getAvailableDimensionsForObjectClass(creatorClass).contains(sourceDimension),
         "bounded FOM object attributes did not expose the shared dimension");
      AttributeHandleSet creatorAttributes = JavaTckSupport.attributeSet(creator, creatorAttribute);
      AttributeHandleSet memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
      AttributeHandleSet endpointAttributes = JavaTckSupport.attributeSet(endpoint, endpointAttribute);
      RegionHandle creatorRegion = null;
      RegionHandle memberRegion = null;
      RegionHandle endpointRegion = null;
      ObjectInstanceHandle object = null;
      AttributeSetRegionSetPairList creatorPair = null;
      AttributeSetRegionSetPairList memberPair = null;
      AttributeSetRegionSetPairList endpointPair = null;
      boolean published = false;
      boolean memberSubscribed = false;
      boolean endpointSubscribed = false;
      boolean associated = false;
      try {
         creator.publishObjectClassAttributes(creatorClass, creatorAttributes);
         published = true;
         creatorRegion = createRegion(creator, sourceDimension, 0, 1);
         memberRegion = createRegion(member, memberDimension, 0, 1);
         endpointRegion = createRegion(endpoint, endpointDimension, 1, 2);
         creatorPair = attributeAssociation(creator, creatorAttributes, creatorRegion);
         memberPair = attributeAssociation(member, memberAttributes, memberRegion);
         endpointPair = attributeAssociation(endpoint, endpointAttributes, endpointRegion);
         member.subscribeObjectClassAttributesWithRegions(memberClass, memberPair);
         memberSubscribed = true;
         endpoint.subscribeObjectClassAttributesWithRegions(endpointClass, endpointPair);
         endpointSubscribed = true;
         object = creator.registerObjectInstanceWithRegions(creatorClass, creatorPair);
         associated = true;
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "bounded default-range object registration returned an invalid handle");
         String registeredName = creator.getObjectInstanceName(object);
         await(member, endpoint, callbackModel,
            () -> !memberCallbacks.discoveries.isEmpty(),
            "discovery in the overlapping [0,1) object subscription");
         settle(member, endpoint, callbackModel);
         ObjectInstanceHandle memberObject = member.getObjectInstanceHandle(registeredName);
         JavaTckSupport.check(memberCallbacks.discoveries.size() == 1
               && memberCallbacks.discoveries.get(0).object.equals(memberObject)
               && registeredName.equals(memberCallbacks.discoveries.get(0).name)
               && endpointCallbacks.discoveries.isEmpty(),
            "object registration at [0,1) crossed the adjacent [1,2) endpoint");

         byte[] value = new byte[] {0x5a};
         AttributeHandleValueMap values =
            creator.getAttributeHandleValueMapFactory().create(1);
         values.put(creatorAttribute, value.clone());
         creator.updateAttributeValues(object, values, new byte[0]);
         await(member, endpoint, callbackModel,
            () -> !memberCallbacks.reflections.isEmpty(),
            "reflection in the overlapping [0,1) object subscription");
         settle(member, endpoint, callbackModel);
         Reflection reflection = memberCallbacks.reflections.get(0);
         JavaTckSupport.check(memberCallbacks.reflections.size() == 1
               && endpointCallbacks.reflections.isEmpty(),
            "object update produced member=" + memberCallbacks.reflections.size()
               + " endpoint=" + endpointCallbacks.reflections.size() + " reflections");
         JavaTckSupport.check(reflection.object.equals(memberObject),
            "the overlapping object update used a different member object handle");
         JavaTckSupport.check(reflection.values.size() == 1
               && Arrays.equals(value, reflection.values.get(memberAttribute)),
            "the overlapping object update changed its attribute value");
         JavaTckSupport.check(reflection.producer.equals(creatorFederate),
            "the overlapping object update changed its producing federate");
         creator.unassociateRegionsForUpdates(object, creatorPair);
         associated = false;
      } finally {
         if (memberSubscribed) try {
            member.unsubscribeObjectClassAttributesWithRegions(memberClass, memberPair);
         } catch (Exception ignored) { }
         if (endpointSubscribed) try {
            endpoint.unsubscribeObjectClassAttributesWithRegions(endpointClass, endpointPair);
         } catch (Exception ignored) { }
         if (associated && object != null) try {
            creator.unassociateRegionsForUpdates(object, creatorPair);
         } catch (Exception ignored) { }
         deleteRegion(member, memberRegion);
         deleteRegion(endpoint, endpointRegion);
         deleteRegion(creator, creatorRegion);
         if (published) try { creator.unpublishObjectClassAttributes(creatorClass, creatorAttributes); }
            catch (Exception ignored) { }
      }
   }

   private static void verifyInteractionRegionRouting(RTIambassador creator, RTIambassador member,
         RTIambassador endpoint, Recorder memberCallbacks, Recorder endpointCallbacks,
         DimensionHandle creatorDimension, long upperBound, FederateHandle creatorFederate,
         CallbackModel callbackModel) throws Exception {
      String interactionName = "HLAinteractionRoot.TckBoundedDefaultInteraction";
      InteractionClassHandle creatorClass = creator.getInteractionClassHandle(interactionName);
      InteractionClassHandle memberClass = member.getInteractionClassHandle(interactionName);
      InteractionClassHandle endpointClass = endpoint.getInteractionClassHandle(interactionName);
      ParameterHandle creatorParameter = creator.getParameterHandle(creatorClass, "Value");
      ParameterHandle memberParameter = member.getParameterHandle(memberClass, "Value");
      ParameterHandle endpointParameter = endpoint.getParameterHandle(endpointClass, "Value");
      DimensionHandle memberDimension = member.getDimensionHandle("TckBoundedDefaultDimension");
      DimensionHandle endpointDimension = endpoint.getDimensionHandle("TckBoundedDefaultDimension");
      JavaTckSupport.check(creatorClass != null && creatorClass.equals(memberClass)
            && creatorClass.equals(endpointClass) && creatorParameter != null
            && creatorParameter.equals(memberParameter) && creatorParameter.equals(endpointParameter)
            && creator.getAvailableDimensionsForInteractionClass(creatorClass)
               .contains(creatorDimension),
         "bounded FOM interaction did not expose its shared dimension and parameter");
      RegionHandle defaultReceiverRegion = null;
      RegionHandle endpointRegion = null;
      RegionHandle sourceRegion = null;
      RegionHandleSet defaultReceiverRegions = null;
      RegionHandleSet endpointRegions = null;
      RegionHandleSet sourceRegions = null;
      boolean published = false;
      boolean memberSubscribed = false;
      boolean endpointSubscribed = false;
      try {
         creator.publishInteractionClass(creatorClass);
         published = true;
         DimensionHandleSet memberDimensions =
            member.getDimensionHandleSetFactory().create();
         memberDimensions.add(memberDimension);
         defaultReceiverRegion = member.createRegion(memberDimensions);
         RangeBounds beforeCommit = member.getRangeBounds(defaultReceiverRegion, memberDimension);
         JavaTckSupport.check(beforeCommit.lower == 0 && beforeCommit.upper == upperBound,
            "the interaction receiver did not receive the FOM-supplied default interval");
         defaultReceiverRegions = regionSet(member, defaultReceiverRegion);
         member.commitRegionModifications(defaultReceiverRegions);
         RangeBounds afterCommit = member.getRangeBounds(defaultReceiverRegion, memberDimension);
         JavaTckSupport.check(afterCommit.lower == 0 && afterCommit.upper == upperBound,
            "committing the interaction receiver changed its FOM-supplied default interval");
         member.subscribeInteractionClassWithRegions(memberClass, defaultReceiverRegions);
         memberSubscribed = true;

         endpointRegion = createRegion(endpoint, endpointDimension, 1, 2);
         endpointRegions = regionSet(endpoint, endpointRegion);
         endpoint.subscribeInteractionClassWithRegions(endpointClass, endpointRegions);
         endpointSubscribed = true;

         sourceRegion = createRegion(creator, creatorDimension, 0, 1);
         sourceRegions = regionSet(creator, sourceRegion);
         byte[] value = new byte[] {(byte) 0xa5};
         byte[] tag = new byte[] {0x71};
         ParameterHandleValueMap parameters =
            creator.getParameterHandleValueMapFactory().create(1);
         parameters.put(creatorParameter, value.clone());
         creator.sendInteractionWithRegions(creatorClass, parameters, sourceRegions, tag.clone());
         await(member, endpoint, callbackModel,
            () -> !memberCallbacks.interactions.isEmpty(),
            "interaction delivery through the FOM-supplied default interval");
         settle(member, endpoint, callbackModel);
         Interaction interaction = memberCallbacks.interactions.get(0);
         JavaTckSupport.check(memberCallbacks.interactions.size() == 1
               && interaction.interaction.equals(memberClass)
               && interaction.parameters.size() == 1
               && Arrays.equals(value, interaction.parameters.get(memberParameter))
               && Arrays.equals(tag, interaction.tag)
               && interaction.producer.equals(creatorFederate)
               && endpointCallbacks.interactions.isEmpty(),
            "the default-range receiver did not get one intact interaction or the [1,2) endpoint matched");
      } finally {
         if (memberSubscribed) try {
            member.unsubscribeInteractionClassWithRegions(memberClass, defaultReceiverRegions);
         } catch (Exception ignored) { }
         if (endpointSubscribed) try {
            endpoint.unsubscribeInteractionClassWithRegions(endpointClass, endpointRegions);
         } catch (Exception ignored) { }
         deleteRegion(member, defaultReceiverRegion);
         deleteRegion(endpoint, endpointRegion);
         deleteRegion(creator, sourceRegion);
         if (published) try { creator.unpublishInteractionClass(creatorClass); }
            catch (Exception ignored) { }
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador,
         DimensionHandle dimension, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(dimension);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, dimension, new RangeBounds(lower, upper));
      RegionHandleSet regions = regionSet(ambassador, region);
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      return regions;
   }

   private static AttributeSetRegionSetPairList attributeAssociation(
         RTIambassador ambassador, AttributeHandleSet attributes, RegionHandle region)
         throws Exception {
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      pairs.add(new AttributeRegionAssociation(attributes, regionSet(ambassador, region)));
      return pairs;
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region) {
      if (region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel callbackModel,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(first, callbackModel);
         pump(second, callbackModel);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void settle(RTIambassador first, RTIambassador second,
         CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         for (int pass = 0; pass < 8; ++pass) {
            pump(first, callbackModel);
            pump(second, callbackModel);
         }
      } else {
         for (int pass = 0; pass < 32; ++pass) {
            Thread.yield();
            Thread.sleep(1L);
         }
      }
   }

   private static void pump(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK dimension callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final String name;

      private Discovery(ObjectInstanceHandle object, String name) {
         this.object = object;
         this.name = name;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new HashMap<>();
      private final FederateHandle producer;

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap incoming = (AttributeHandleValueMap) arguments[1];
         incoming.forEach((handle, value) -> values.put(handle, value.clone()));
         producer = (FederateHandle) arguments[4];
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      private final byte[] tag;
      private final FederateHandle producer;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap incoming = (ParameterHandleValueMap) arguments[1];
         incoming.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (String) arguments[2]));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection(arguments));
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK FOM dimension route recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
