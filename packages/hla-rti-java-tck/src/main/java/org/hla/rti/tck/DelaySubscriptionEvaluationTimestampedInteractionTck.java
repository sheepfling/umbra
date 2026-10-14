package org.hla.rti.tck;

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
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Re-evaluate timestamped interaction subscriptions at the time-grant boundary. */
final class DelaySubscriptionEvaluationTimestampedInteractionTck {
   private DelaySubscriptionEvaluationTimestampedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String interactionClassName, String parameterName,
         CallbackModel model) throws Exception {
      runConfiguration(factory, fom, switchesFom, timeImplementation,
         interactionClassName, parameterName, model, false);
      runConfiguration(factory, fom, switchesFom, timeImplementation,
         interactionClassName, parameterName, model, true);
   }

   static void runRegional(RtiFactory factory, String switchesFom, String ddmFom,
         String timeImplementation, String interactionClassName, String parameterName,
         String firstDimensionName, String secondDimensionName, CallbackModel model)
         throws Exception {
      runRegionalConfiguration(factory, switchesFom, ddmFom, timeImplementation,
         interactionClassName, parameterName, firstDimensionName, secondDimensionName,
         model, false);
      runRegionalConfiguration(factory, switchesFom, ddmFom, timeImplementation,
         interactionClassName, parameterName, firstDimensionName, secondDimensionName,
         model, true);
   }

   private static void runRegionalConfiguration(RtiFactory factory, String switchesFom,
         String ddmFom, String timeImplementation, String interactionClassName,
         String parameterName, String firstDimensionName, String secondDimensionName,
         CallbackModel model, boolean enabled) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-delay-subscription-regional-timestamped-interaction-"
         + enabled + "-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean regulating = false, constrained = false, published = false, subscribed = false;
      RegionHandle sourceRegion = null, receiverRegion = null;
      RegionHandleSet sourceRegions = null, receiverRegions = null;
      InteractionClassHandle publisherClass = null, receiverClass = null;
      ParameterHandle publisherParameter = null, receiverParameter = null;
      try {
         publisher.connect(publisherEvents.proxy(), model); publisherConnected = true;
         receiver.connect(receiverEvents.proxy(), model); receiverConnected = true;
         String[] modules = enabled ? new String[] {ddmFom, switchesFom} : new String[] {ddmFom};
         publisher.createFederationExecution(federation, modules, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-regional-dse-interaction-publisher", federation); publisherJoined = true;
         receiver.joinFederationExecution("java-tck-regional-dse-interaction-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getInteractionClassHandle(interactionClassName);
         receiverClass = receiver.getInteractionClassHandle(interactionClassName);
         publisherParameter = publisher.getParameterHandle(publisherClass, parameterName);
         receiverParameter = receiver.getParameterHandle(receiverClass, parameterName);
         DimensionHandle publisherFirst = publisher.getDimensionHandle(firstDimensionName);
         DimensionHandle publisherSecond = publisher.getDimensionHandle(secondDimensionName);
         DimensionHandle receiverFirst = receiver.getDimensionHandle(firstDimensionName);
         DimensionHandle receiverSecond = receiver.getDimensionHandle(secondDimensionName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherParameter.equals(receiverParameter)
               && publisherFirst.equals(receiverFirst) && publisherSecond.equals(receiverSecond)
               && firstDimensionName.equals(publisher.getDimensionName(publisherFirst))
               && secondDimensionName.equals(publisher.getDimensionName(publisherSecond)),
            "adapter DDM FOM returned inconsistent regional interaction handles or dimensions");
         checkInteractionDimensions(publisher, publisherClass, publisherFirst, publisherSecond);
         checkInteractionDimensions(receiver, receiverClass, receiverFirst, receiverSecond);
         JavaTckSupport.check(publisher.getDelaySubscriptionEvaluationSwitch() == enabled
               && receiver.getDelaySubscriptionEvaluationSwitch() == enabled,
            "regional interaction Delay Subscription Evaluation switch did not match composed FOM modules");

         publisher.publishInteractionClass(publisherClass); published = true;
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);
         receiver.setConveyRegionDesignatorSetsSwitch(true);
         JavaTckSupport.check(receiver.getConveyRegionDesignatorSetsSwitch(),
            "regional interaction receiver did not enable region designators");
         sourceRegion = createRegion(publisher, publisherFirst, publisherSecond, 1, 5);
         receiverRegion = createRegion(receiver, receiverFirst, receiverSecond, 2, 6);
         sourceRegions = regionSet(publisher, sourceRegion);
         receiverRegions = regionSet(receiver, receiverRegion);

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "regional interaction members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon); regulating = true;
         await(model, publisher, interactionClassName,
            () -> publisherEvents.timeRegulationEnabled != null, "Time Regulation enablement");
         receiver.enableTimeConstrained(); constrained = true;
         await(model, receiver, interactionClassName,
            () -> receiverEvents.timeConstrainedEnabled != null, "Time Constrained enablement");

         LogicalTime firstTime = add(publisher.queryLogicalTime(), intervalOffset(epsilon, 2));
         LogicalTime firstReceiverTime = add(
            receiver.queryLogicalTime(), intervalOffset(epsilon, 2));
         JavaTckSupport.check(firstTime.equals(firstReceiverTime),
            "regional interaction members derived different first grant times");
         byte[] firstValue = new byte[] {0x44, 0x53, 0x52, 0x49};
         byte[] secondValue = new byte[] {0x44, 0x53, 0x52, 0x32};
         byte[] tag = new byte[] {0x44, 0x53, 0x52, 0x54};
         ParameterHandleValueMap firstParameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         firstParameters.put(publisherParameter, firstValue.clone());
         MessageRetractionReturn firstRetraction = publisher.sendInteractionWithRegions(
            publisherClass, firstParameters, sourceRegions, tag.clone(), firstTime);
         JavaTckSupport.check(!enabled || (firstRetraction != null
               && firstRetraction.retractionHandleIsValid && firstRetraction.handle != null),
            "enabled regional timestamped interaction returned an invalid retraction handle");
         JavaTckSupport.check(receiverEvents.interactions.isEmpty(),
            "regional timestamped interaction arrived before the constrained grant");
         receiver.subscribeInteractionClassWithRegions(receiverClass, receiverRegions);
         subscribed = true;
         receiver.timeAdvanceRequest(firstReceiverTime);
         publisher.timeAdvanceRequest(firstTime);
         awaitPair(model, publisher, receiver, interactionClassName,
            () -> publisherEvents.grants.size() >= 1 && receiverEvents.grants.size() >= 1
               && receiverEvents.interactions.size() >= (enabled ? 1 : 0),
            "first regional delay-subscription grant");
         int firstDeliveryCount = receiverEvents.interactions.size();
         JavaTckSupport.check(enabled ? firstDeliveryCount == 1 : firstDeliveryCount <= 1,
            "regional Delay Subscription Evaluation delivered an unexpected first-interaction count");
         if (firstDeliveryCount > 0) {
            MessageRetractionHandle expectedRetraction = firstRetraction == null
               ? null : firstRetraction.handle;
            verifyRegional(receiverEvents.interactions.get(0), receiverClass,
               receiverParameter, firstValue, tag, producer, firstTime, expectedRetraction,
               sourceRegion, receiver, "first regional grant");
         }

         LogicalTime secondTime = add(firstTime, epsilon);
         LogicalTime secondReceiverTime = add(firstReceiverTime, epsilon);
         ParameterHandleValueMap secondParameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         secondParameters.put(publisherParameter, secondValue.clone());
         MessageRetractionReturn secondRetraction = publisher.sendInteractionWithRegions(
            publisherClass, secondParameters, sourceRegions, tag.clone(), secondTime);
         JavaTckSupport.check(secondRetraction != null
               && secondRetraction.retractionHandleIsValid && secondRetraction.handle != null,
            "regional timestamped interaction returned an invalid second retraction handle");
         receiver.unsubscribeInteractionClassWithRegions(receiverClass, receiverRegions);
         subscribed = false;
         receiver.timeAdvanceRequest(secondReceiverTime);
         publisher.timeAdvanceRequest(secondTime);
         awaitPair(model, publisher, receiver, interactionClassName,
            () -> publisherEvents.grants.size() >= 2 && receiverEvents.grants.size() >= 2,
            "second regional delay-subscription grant");
         JavaTckSupport.check(receiverEvents.interactions.size() == firstDeliveryCount,
            "regional timestamped interaction was delivered after unsubscribe");

         receiver.disableTimeConstrained(); constrained = false;
         publisher.disableTimeRegulation(); regulating = false;
         publisher.unpublishInteractionClass(publisherClass); published = false;
         receiver.deleteRegion(receiverRegion); receiverRegion = null;
         publisher.deleteRegion(sourceRegion); sourceRegion = null;
         receiver.resignFederationExecution(ResignAction.NO_ACTION); receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION); publisherJoined = false;
         publisher.destroyFederationExecution(federation); created = false;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined && receiverRegions != null) try {
            receiver.unsubscribeInteractionClassWithRegions(receiverClass, receiverRegions);
         } catch (Exception ignored) { }
         if (published && publisherJoined) try { publisher.unpublishInteractionClass(publisherClass); }
            catch (Exception ignored) { }
         deleteRegion(receiver, receiverRegion, receiverJoined);
         deleteRegion(publisher, sourceRegion, publisherJoined);
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void runConfiguration(RtiFactory factory, String fom, String switchesFom,
         String timeImplementation, String interactionClassName, String parameterName,
         CallbackModel model, boolean enabled) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-delay-subscription-timestamped-interaction-"
         + enabled + "-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         publisherConnected = true;
         receiver.connect(receiverEvents.proxy(), model);
         receiverConnected = true;
         String[] modules = enabled ? new String[] {fom, switchesFom} : new String[] {fom};
         publisher.createFederationExecution(federation, modules, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-delay-tso-interaction-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-delay-tso-interaction-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherClass =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverClass =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherClass, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverClass, parameterName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherParameter.equals(receiverParameter),
            "adapter FOM returned inconsistent interaction/parameter handles");
         JavaTckSupport.check(publisher.getDelaySubscriptionEvaluationSwitch() == enabled
               && receiver.getDelaySubscriptionEvaluationSwitch() == enabled,
            "Delay Subscription Evaluation switch did not match the composed FOM");
         publisher.publishInteractionClass(publisherClass);
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon);
         await(model, publisher, interactionClassName,
            () -> publisherEvents.timeRegulationEnabled != null,
            "time-regulation enablement");
         receiver.enableTimeConstrained();
         await(model, receiver, interactionClassName,
            () -> receiverEvents.timeConstrainedEnabled != null,
            "time-constrained enablement");

         LogicalTime firstTime = add(publisher.queryLogicalTime(), intervalOffset(epsilon, 2));
         LogicalTime firstReceiverTime = add(
            receiver.queryLogicalTime(), intervalOffset(epsilon, 2));
         JavaTckSupport.check(firstTime.equals(firstReceiverTime),
            "federates derived different first grant times");
         byte[] payload = new byte[] {(byte) 0xA1};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         MessageRetractionReturn firstRetraction = publisher.sendInteraction(
            publisherClass, parameters, new byte[0], firstTime);
         JavaTckSupport.check(!enabled || (firstRetraction != null
               && firstRetraction.retractionHandleIsValid && firstRetraction.handle != null),
            "enabled timestamped interaction returned an invalid retraction handle");
         receiver.subscribeInteractionClass(receiverClass);
         receiver.timeAdvanceRequest(firstReceiverTime);
         publisher.timeAdvanceRequest(firstTime);
         awaitPair(model, publisher, receiver, interactionClassName,
            () -> publisherEvents.grants.size() >= 1 && receiverEvents.grants.size() >= 1
               && receiverEvents.interactions.size() >= (enabled ? 1 : 0),
            "first timestamped interaction grant");

         int firstDeliveryCount = receiverEvents.interactions.size();
         JavaTckSupport.check(enabled ? firstDeliveryCount == 1 : firstDeliveryCount <= 1,
            "unexpected first timestamped interaction count");
         if (firstDeliveryCount != 0) {
            JavaTckSupport.check(firstRetraction != null && firstRetraction.handle != null,
               "delivered timestamped interaction has no matching retraction designator");
            verify(receiverEvents.interactions.get(0), receiverClass, receiverParameter,
               payload, producer, firstTime, firstRetraction.handle, receiver,
               "first grant");
         }

         LogicalTime secondTime = add(firstTime, epsilon);
         LogicalTime secondReceiverTime = add(firstReceiverTime, epsilon);
         MessageRetractionReturn secondRetraction = publisher.sendInteraction(
            publisherClass, parameters, new byte[0], secondTime);
         JavaTckSupport.check(secondRetraction != null
               && secondRetraction.retractionHandleIsValid && secondRetraction.handle != null,
            "second timestamped interaction returned an invalid retraction handle");
         receiver.unsubscribeInteractionClass(receiverClass);
         receiver.timeAdvanceRequest(secondReceiverTime);
         publisher.timeAdvanceRequest(secondTime);
         awaitPair(model, publisher, receiver, interactionClassName,
            () -> publisherEvents.grants.size() >= 2 && receiverEvents.grants.size() >= 2,
            "second timestamped interaction grant");
         JavaTckSupport.check(receiverEvents.interactions.size() == firstDeliveryCount,
            "timestamped interaction arrived after unsubscribe");

         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static LogicalTime add(LogicalTime start, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) start.add(interval);
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static void verify(Interaction event, InteractionClassHandle interactionClass,
         ParameterHandle parameter, byte[] payload, FederateHandle producer, LogicalTime time,
         MessageRetractionHandle retraction, RTIambassador receiver, String phase)
         throws Exception {
      JavaTckSupport.check(interactionClass.equals(event.interaction)
            && event.parameters.size() == 1
            && event.parameters.containsKey(parameter)
            && Arrays.equals(payload, event.parameters.get(parameter))
            && event.tag.length == 0
            && producer.equals(event.producer)
            && time.equals(event.time)
            && event.sentOrder == OrderType.TIMESTAMP
            && event.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(event.retraction)
            && event.transportation != null
            && !receiver.getTransportationTypeName(event.transportation).isEmpty()
            && event.regionsEmpty,
         phase + " timestamped interaction lost standard delivery metadata");
   }

   private static void checkInteractionDimensions(RTIambassador ambassador,
         InteractionClassHandle interactionClass, DimensionHandle first,
         DimensionHandle second) throws Exception {
      java.util.Set<DimensionHandle> available =
         ambassador.getAvailableDimensionsForInteractionClass(interactionClass);
      JavaTckSupport.check(available.contains(first) && available.contains(second)
            && ambassador.getDimensionUpperBound(first) >= 6
            && ambassador.getDimensionUpperBound(second) >= 6,
         "adapter DDM FOM does not expose the configured interaction dimensions");
   }

   private static RegionHandle createRegion(RTIambassador ambassador, DimensionHandle first,
         DimensionHandle second, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(first);
      dimensions.add(second);
      RegionHandle region = ambassador.createRegion(dimensions);
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

   private static void verifyRegional(Interaction event,
         InteractionClassHandle interactionClass, ParameterHandle parameter, byte[] payload,
         byte[] tag, FederateHandle producer, LogicalTime time,
         MessageRetractionHandle expectedRetraction, RegionHandle sourceRegion,
         RTIambassador receiver, String phase) throws Exception {
      JavaTckSupport.check(interactionClass.equals(event.interaction)
            && event.parameters.size() == 1 && event.parameters.containsKey(parameter)
            && Arrays.equals(payload, event.parameters.get(parameter))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && time.equals(event.time) && event.sentOrder == OrderType.TIMESTAMP
            && event.receivedOrder == OrderType.TIMESTAMP
            && event.transportation != null
            && !receiver.getTransportationTypeName(event.transportation).isEmpty()
            && event.regions != null && event.regions.size() == 1
            && event.regions.contains(sourceRegion),
         phase + " regional timestamped interaction lost metadata or source designator");
      if (expectedRetraction != null) {
         JavaTckSupport.check(expectedRetraction.equals(event.retraction),
            phase + " regional timestamped interaction returned the wrong retraction handle");
      }
   }

   private static void deleteRegion(RTIambassador ambassador, RegionHandle region,
         boolean joined) {
      if (joined && region != null) try { ambassador.deleteRegion(region); }
         catch (Exception ignored) { }
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String interactionClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getInteractionClassHandle(interactionClassName);
   }

   private static void await(CallbackModel model, RTIambassador ambassador,
         String interactionClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(ambassador, model, interactionClassName);
         if (condition.isTrue()) return;
         Thread.yield();
      }
      throw new AssertionError("timed out waiting for " + description);
   }

   private static void awaitPair(CallbackModel model, RTIambassador publisher,
         RTIambassador receiver, String interactionClassName, Condition condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(publisher, model, interactionClassName);
         service(receiver, model, interactionClassName);
         if (condition.isTrue()) return;
         Thread.yield();
      }
      throw new AssertionError("timed out waiting for " + description);
   }

   @FunctionalInterface
   private interface Condition { boolean isTrue() throws Exception; }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Interaction {
      InteractionClassHandle interaction;
      ParameterHandleValueMap parameters;
      byte[] tag;
      TransportationTypeHandle transportation;
      FederateHandle producer;
      boolean regionsEmpty;
      LogicalTime<?, ?> time;
      OrderType sentOrder;
      OrderType receivedOrder;
      MessageRetractionHandle retraction;
      RegionHandleSet regions;
   }

   private static final class Recorder {
      LogicalTime<?, ?> timeRegulationEnabled;
      LogicalTime<?, ?> timeConstrainedEnabled;
      final List<Interaction> interactions = new ArrayList<>();
      final List<LogicalTime<?, ?>> grants = new ArrayList<>();

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               Object[] a = args == null ? new Object[0] : args;
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name) && a.length == 1) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) a[0];
               } else if ("timeConstrainedEnabled".equals(name) && a.length == 1) {
                  timeConstrainedEnabled = (LogicalTime<?, ?>) a[0];
               } else if ("receiveInteraction".equals(name) && a.length == 10) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) a[0];
                  event.parameters = ((ParameterHandleValueMap) a[1]).clone();
                  event.tag = ((byte[]) a[2]).clone();
                  event.transportation = (TransportationTypeHandle) a[3];
                  event.producer = (FederateHandle) a[4];
                  RegionHandleSet regions = (RegionHandleSet) a[5];
                  event.regionsEmpty = regions == null || regions.isEmpty();
                  event.regions = regions;
                  event.time = (LogicalTime<?, ?>) a[6];
                  event.sentOrder = (OrderType) a[7];
                  event.receivedOrder = (OrderType) a[8];
                  event.retraction = (MessageRetractionHandle) a[9];
                  interactions.add(event);
               } else if ("timeAdvanceGrant".equals(name) && a.length == 1) {
                  grants.add((LogicalTime<?, ?>) a[0]);
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped DSE callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == a[0];
               }
               return null;
            });
      }
   }
}
