package org.hla.rti.tck;

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
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;

/** Verifies that callback controls defer timestamped regional interaction delivery and grants. */
final class CallbackControlsTimestampedRegionalInteractionTck {
   private CallbackControlsTimestampedRegionalInteractionTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String firstDimensionName,
         String secondDimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-callback-tso-regional-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean regulating = false;
      boolean constrained = false;
      boolean callbacksDisabled = false;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      ParameterHandle publisherParameter = null;
      ParameterHandle receiverParameter = null;
      RegionHandle sourceRegion = null;
      RegionHandle receiverRegion = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherFederate =
            publisher.joinFederationExecution("java-tck-tso-region-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-tso-region-receiver", federation);
         receiverJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter)
               && publisherFirst.equals(receiverFirst) && publisherSecond.equals(receiverSecond),
            "adapter DDM FOM lookups returned inconsistent interaction or dimension handles");
         JavaTckSupport.check(publisher.getAvailableDimensionsForInteractionClass(
               publisherInteraction).contains(publisherFirst)
               && publisher.getAvailableDimensionsForInteractionClass(publisherInteraction)
                  .contains(publisherSecond),
            "adapter interaction class does not expose both configured dimensions");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6, 2, 6);
         RegionHandleSet sourceRegions = regionSet(publisher, sourceRegion);
         RegionHandleSet receiverRegions = regionSet(receiver, receiverRegion);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "convey-region-designator-sets switch did not turn on");
         receiver.subscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(receiverTimeFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "federates began at different logical times");
         LogicalTimeInterval<?> epsilon =
            (LogicalTimeInterval<?>) publisherTimeFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         constrained = true;
         await(receiver, callbackModel, interactionClassName,
            () -> receiverRecorder.timeConstrainedEnabled,
            "receiver time-constrained callback");
         publisher.enableTimeRegulation(epsilon);
         regulating = true;
         await(publisher, callbackModel, interactionClassName,
            () -> publisherRecorder.timeRegulationEnabled,
            "publisher time-regulation callback");
         publisherRecorder.clearApplicationEvents();
         receiverRecorder.clearApplicationEvents();

         LogicalTime<?, ?> interactionTime = addSteps(publisherInitial, epsilon, 4);
         LogicalTime<?, ?> receiverTarget = addSteps(receiverInitial, epsilon, 4);
         JavaTckSupport.check(interactionTime.equals(receiverTarget),
            "timestamped interaction and receiver advance targets differ");
         byte[] parameterBytes = new byte[] {0x52, 0x49, 0x47};
         byte[] tag = new byte[] {0x52, 0x47, 0x43};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, parameterBytes.clone());

         receiver.disableCallbacks();
         callbacksDisabled = true;
         MessageRetractionReturn sent = publisher.sendInteractionWithRegions(
            publisherInteraction, parameters, sourceRegions, tag, interactionTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped regional send returned an invalid retraction handle");

         receiver.timeAdvanceRequest(receiverTarget);
         publisher.timeAdvanceRequest(interactionTime);
         await(publisher, callbackModel, interactionClassName,
            () -> publisherRecorder.grants.size() == 1,
            "publisher timestamped-interaction time-advance grant");
         serviceReceiver(receiver, callbackModel, interactionClassName);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty()
               && receiverRecorder.grants.isEmpty(),
            "disabled callbacks exposed the timestamped interaction or its grant");

         receiver.enableCallbacks();
         callbacksDisabled = false;
         serviceReceiver(receiver, callbackModel, interactionClassName);
         await(receiver, callbackModel, interactionClassName,
            () -> receiverRecorder.interactions.size() == 1
               && receiverRecorder.grants.size() == 1,
            "re-enabled timestamped regional interaction and grant");
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1,
            "re-enabled timestamped regional interaction was duplicated");
         TimedInteraction delivered = receiverRecorder.interactions.get(0);
         TransportationTypeHandle transportation = delivered.transportation;
         JavaTckSupport.check(delivered.interaction.equals(receiverInteraction)
               && delivered.parameters.size() == 1
               && delivered.parameters.containsKey(receiverParameter)
               && Arrays.equals(parameterBytes, delivered.parameters.get(receiverParameter))
               && Arrays.equals(tag, delivered.tag)
               && publisherFederate.equals(delivered.producer)
               && interactionTime.equals(delivered.time)
               && delivered.sentOrder == OrderType.TIMESTAMP
               && delivered.receivedOrder == OrderType.TIMESTAMP
               && transportation != null
               && !receiver.getTransportationTypeName(transportation).isEmpty()
               && sent.handle.equals(delivered.retraction)
               && delivered.retraction != null && delivered.retraction.encodedLength() > 0
               && delivered.regions != null && delivered.regions.size() == 1
               && delivered.regions.contains(sourceRegion),
            "re-enabled regional interaction returned incorrect timestamped metadata");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "timestamped interaction did not precede the grant callback");
         JavaTckSupport.check(receiverTarget.equals(receiverRecorder.grants.get(0))
               && receiverTarget.equals(receiver.queryLogicalTime()),
            "receiver grant or logical-time query did not match its requested target");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         receiver.unsubscribeInteractionClassWithRegions(receiverInteraction, receiverRegions);
         subscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         receiver.deleteRegion(receiverRegion);
         receiverRegion = null;
         publisher.deleteRegion(sourceRegion);
         sourceRegion = null;
      } finally {
         if (callbacksDisabled && receiverJoined) {
            try { receiver.enableCallbacks(); } catch (Exception ignored) { }
         }
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined && receiverInteraction != null && receiverRegion != null) {
            try {
               receiver.unsubscribeInteractionClassWithRegions(receiverInteraction,
                  regionSet(receiver, receiverRegion));
            } catch (Exception ignored) { }
         }
         if (published && publisherJoined && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (receiverRegion != null && receiverJoined) {
            try { receiver.deleteRegion(receiverRegion); } catch (Exception ignored) { }
         }
         if (sourceRegion != null && publisherJoined) {
            try { publisher.deleteRegion(sourceRegion); } catch (Exception ignored) { }
         }
         if (receiverJoined) resign(receiver, ResignAction.NO_ACTION);
         if (publisherJoined) resign(publisher, ResignAction.NO_ACTION);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static RegionHandle createRegion(RTIambassador ambassador,
         DimensionHandle firstDimension, DimensionHandle secondDimension,
         long firstLower, long firstUpper, long secondLower, long secondUpper) throws Exception {
      JavaTckSupport.check(ambassador.getDimensionUpperBound(firstDimension) >= firstUpper
            && ambassador.getDimensionUpperBound(secondDimension) >= secondUpper,
         "adapter dimensions do not contain the configured interaction region");
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(firstDimension);
      dimensions.add(secondDimension);
      RegionHandle region = ambassador.createRegion(dimensions);
      RegionHandleSet regions = regionSet(ambassador, region);
      ambassador.setRangeBounds(region, firstDimension,
         new RangeBounds(firstLower, firstUpper));
      ambassador.setRangeBounds(region, secondDimension,
         new RangeBounds(secondLower, secondUpper));
      ambassador.commitRegionModifications(regions);
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle region)
         throws Exception {
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      regions.add(region);
      return regions;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime addSteps(LogicalTime start, LogicalTimeInterval interval,
         int count) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < count; ++i) result = result.add(interval);
      return result;
   }

   private static void serviceReceiver(RTIambassador receiver, CallbackModel callbackModel,
         String interactionClassName) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         receiver.evokeMultipleCallbacks(0.0, 1.0);
      } else {
         receiver.getInteractionClassHandle(interactionClassName);
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         String interactionClassName, java.util.function.BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         serviceReceiver(ambassador, callbackModel, interactionClassName);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class TimedInteraction {
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

      private TimedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap receivedParameters = (ParameterHandleValueMap) arguments[1];
         receivedParameters.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regions = arguments[5] == null ? null : new HashSet<>((RegionHandleSet) arguments[5]);
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<TimedInteraction> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  interactions.add(new TimedInteraction(arguments));
                  callbackOrder.add("receiveInteraction");
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("timeAdvanceGrant");
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK timestamped regional interaction callbacks";
                  }
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearApplicationEvents() {
         grants.clear();
         interactions.clear();
         callbackOrder.clear();
      }
   }
}
