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
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks adapter DDM handles and receive-order delivery through the void Java send overload. */
final class RegionalSendNoRetractionDesignatorTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;

   private RegionalSendNoRetractionDesignatorTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName, String attributeName,
         String interactionName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-regional-send-no-retraction-" + suffix;
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean federationCreated = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      ParameterHandle publisherParameter = null;
      ParameterHandle receiverParameter = null;
      RegionHandle sourceRegion = null;
      RegionHandle receiverRegion = null;
      try {
         publisher.connect(noopFederateAmbassador(), callbackModel);
         publisherConnected = true;
         receiver.connect(recorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom);
         federationCreated = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-regional-send-publisher-" + suffix, federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-regional-send-receiver-" + suffix, federation);
         receiverJoined = true;

         ObjectClassHandle publisherObject = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverObject = receiver.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(publisherObject != null && receiverObject != null,
            "regional send object-class lookup returned no handle");
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherObject, attributeName);
         AttributeHandle receiverAttribute =
            receiver.getAttributeHandle(receiverObject, attributeName);
         JavaTckSupport.check(publisherAttribute != null && receiverAttribute != null,
            "regional send attribute lookup returned no handle");
         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(publisherInteraction != null && receiverInteraction != null,
            "regional send interaction lookup returned no handle");
         publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);
         JavaTckSupport.check(publisherParameter != null && receiverParameter != null,
            "regional send parameter lookup returned no handle");
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherFirst != null && publisherSecond != null
               && receiverFirst != null && receiverSecond != null,
            "regional send dimension lookup returned no handle");
         JavaTckSupport.check(attributeName.equals(publisher.getAttributeName(
                  publisherObject, publisherAttribute))
               && attributeName.equals(receiver.getAttributeName(
                  receiverObject, receiverAttribute))
               && objectClassName.equals(publisher.getObjectClassName(publisherObject))
               && objectClassName.equals(receiver.getObjectClassName(receiverObject))
               && publisherObject.equals(receiverObject)
               && publisherAttribute.equals(receiverAttribute)
               && interactionName.equals(
               publisher.getInteractionClassName(publisherInteraction))
               && interactionName.equals(receiver.getInteractionClassName(receiverInteraction))
               && parameterName.equals(publisher.getParameterName(
                  publisherInteraction, publisherParameter))
               && parameterName.equals(receiver.getParameterName(
                  receiverInteraction, receiverParameter))
               && publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            "regional send adapter names did not resolve to matching class and member handles");
         JavaTckSupport.check(!publisherFirst.equals(publisherSecond)
               && !receiverFirst.equals(receiverSecond)
               && publisher.getAvailableDimensionsForObjectClass(publisherObject)
               .containsAll(Arrays.asList(publisherFirst, publisherSecond))
               && receiver.getAvailableDimensionsForObjectClass(receiverObject)
                  .containsAll(Arrays.asList(receiverFirst, receiverSecond))
               && publisher.getAvailableDimensionsForInteractionClass(
                  publisherInteraction).containsAll(Arrays.asList(publisherFirst, publisherSecond))
               && receiver.getAvailableDimensionsForInteractionClass(
                  receiverInteraction).containsAll(Arrays.asList(receiverFirst, receiverSecond))
               && firstDimensionName.equals(publisher.getDimensionName(publisherFirst))
               && secondDimensionName.equals(publisher.getDimensionName(publisherSecond))
               && firstDimensionName.equals(receiver.getDimensionName(receiverFirst))
               && secondDimensionName.equals(receiver.getDimensionName(receiverSecond))
               && publisher.getDimensionUpperBound(publisherFirst) >= 4
               && publisher.getDimensionUpperBound(publisherSecond) >= 4
               && receiver.getDimensionUpperBound(receiverFirst) >= 4
               && receiver.getDimensionUpperBound(receiverSecond) >= 4,
            "adapter DDM FOM must expose both object and interaction dimensions with names and sufficient ranges");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6, 2, 6);
         RegionHandleSet receiverRegions = regionSet(receiver, receiverRegion);
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = true;

         Class<?> returnType = RTIambassador.class.getMethod("sendInteractionWithRegions",
            InteractionClassHandle.class, ParameterHandleValueMap.class,
            RegionHandleSet.class, byte[].class).getReturnType();
         JavaTckSupport.check(returnType == void.class,
            "the no-time sendInteractionWithRegions overload exposed a retraction return value");

         byte[] payload = new byte[] {(byte) 0x6e, (byte) 0x6c};
         byte[] tag = new byte[] {(byte) 0x4e, (byte) 0x52};
         ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload.clone());
         publisher.sendInteractionWithRegions(publisherInteraction, parameters,
            regionSet(publisher, sourceRegion), tag.clone());

         await(receiver, callbackModel, recorder,
            "ordinary regional interaction delivery without a retraction designator");
         JavaTckSupport.check(recorder.interactions.size() == 1
               && recorder.timedInteractions.isEmpty(),
            "ordinary regional send did not produce exactly one receive-order interaction");
         ReceivedInteraction received = recorder.interactions.get(0);
         JavaTckSupport.check(received.interaction.equals(receiverInteraction)
               && received.parameters.size() == 1
               && Arrays.equals(payload, received.parameters.get(receiverParameter))
               && Arrays.equals(tag, received.tag)
               && received.producer.equals(producer),
            "ordinary regional send did not deliver the expected receive-order interaction");
      } finally {
         if (subscribed && receiverJoined && receiverInteraction != null && receiverRegion != null) {
            try { receiver.unsubscribeInteractionClassWithRegions(
               receiverInteraction, regionSet(receiver, receiverRegion)); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (sourceRegion != null && publisherJoined) {
            try { publisher.deleteRegion(sourceRegion); }
            catch (Exception ignored) { }
         }
         if (receiverRegion != null && receiverJoined) {
            try { receiver.deleteRegion(receiverRegion); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (federationCreated) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador,
         DimensionHandle firstDimension, DimensionHandle secondDimension,
         long firstLower, long firstUpper, long secondLower, long secondUpper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(firstDimension);
      dimensions.add(secondDimension);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, firstDimension,
         new hla.rti1516_2025.RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, secondDimension,
         new hla.rti1516_2025.RangeBounds(secondLower, secondUpper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle... regions)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      for (RegionHandle region : regions) result.add(region);
      return result;
   }

   private static void await(RTIambassador receiver, CallbackModel callbackModel,
         Recorder recorder, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && recorder.interactions.isEmpty()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(receiver);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(!recorder.interactions.isEmpty(), "timed out waiting for " + description);
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         RegionalSendNoRetractionDesignatorTck::objectMethod);
   }

   private static Object objectMethod(Object proxy, java.lang.reflect.Method method,
         Object[] arguments) {
      if (method.getDeclaringClass() == Object.class) {
         if ("toString".equals(method.getName())) return "Java TCK regional send callbacks";
         if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
         if ("equals".equals(method.getName())) return proxy == arguments[0];
      }
      return null;
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class ReceivedInteraction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;

      private ReceivedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
         values.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
      }
   }

   private static final class Recorder {
      private final CopyOnWriteArrayList<ReceivedInteraction> interactions =
         new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<Object[]> timedInteractions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null) {
                  if (arguments.length == 6) interactions.add(new ReceivedInteraction(arguments));
                  else if (arguments.length == 10) timedInteractions.add(arguments.clone());
               }
               return objectMethod(proxy, method, arguments);
            });
      }
   }
}
