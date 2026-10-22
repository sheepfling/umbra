package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks smallest-feasible implicit advance while enabling Time Regulation. */
final class TimeRegulationImplicitAdvanceTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private TimeRegulationImplicitAdvanceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador producer = factory.getRtiAmbassador();
      RTIambassador enabling = factory.getRtiAmbassador();
      RTIambassador advancedPeer = factory.getRtiAmbassador();
      Recorder producerCallbacks = new Recorder();
      Recorder enablingCallbacks = new Recorder();
      Recorder peerCallbacks = new Recorder();
      String federation = "java-tck-time-regulation-implicit-advance-" + UUID.randomUUID();
      boolean producerConnected = false, enablingConnected = false, peerConnected = false;
      boolean federationCreated = false;
      boolean producerJoined = false, enablingJoined = false, peerJoined = false;
      InteractionClassHandle producerClass = null, enablingClass = null;
      boolean published = false, subscribed = false;
      try {
         producer.connect(producerCallbacks.proxy(), callbackModel);
         producerConnected = true;
         enabling.connect(enablingCallbacks.proxy(), callbackModel);
         enablingConnected = true;
         advancedPeer.connect(peerCallbacks.proxy(), callbackModel);
         peerConnected = true;
         producer.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         FederateHandle producerHandle = producer.joinFederationExecution(
            "java-tck-implicit-advance-producer", federation);
         producerJoined = true;
         enabling.joinFederationExecution("java-tck-implicit-advance-member", federation);
         enablingJoined = true;
         advancedPeer.joinFederationExecution("java-tck-implicit-advance-peer", federation);
         peerJoined = true;

         producerClass = producer.getInteractionClassHandle(interactionName);
         enablingClass = enabling.getInteractionClassHandle(interactionName);
         ParameterHandle producerParameter = producer.getParameterHandle(
            producerClass, parameterName);
         ParameterHandle enablingParameter = enabling.getParameterHandle(
            enablingClass, parameterName);
         JavaTckSupport.check(producerClass != null && enablingClass != null
               && producerParameter != null && enablingParameter != null,
            "adapter interaction FOM did not resolve the implicit-advance handles");
         producer.publishInteractionClass(producerClass);
         published = true;
         producer.changeInteractionOrderType(producerClass, OrderType.TIMESTAMP);
         enabling.subscribeInteractionClass(enablingClass);
         subscribed = true;

         LogicalTimeFactory<?, ?> producerFactory = producer.getTimeFactory();
         LogicalTimeFactory<?, ?> enablingFactory = enabling.getTimeFactory();
         LogicalTimeFactory<?, ?> peerFactory = advancedPeer.getTimeFactory();
         JavaTckSupport.check(producerFactory.getName().equals(enablingFactory.getName())
               && producerFactory.getName().equals(peerFactory.getName()),
            "implicit-advance members selected different logical-time factories");
         LogicalTimeInterval producerEpsilon =
            (LogicalTimeInterval) producerFactory.makeEpsilon();
         LogicalTimeInterval enablingEpsilon =
            (LogicalTimeInterval) enablingFactory.makeEpsilon();
         LogicalTimeInterval peerEpsilon = (LogicalTimeInterval) peerFactory.makeEpsilon();
         LogicalTime<?, ?> producerInitial = producer.queryLogicalTime();
         LogicalTime<?, ?> enablingInitial = enabling.queryLogicalTime();
         LogicalTime<?, ?> peerInitial = advancedPeer.queryLogicalTime();
         JavaTckSupport.check(producerInitial.equals(enablingInitial)
               && producerInitial.equals(peerInitial),
            "implicit-advance members did not start at the same logical time");

         enabling.enableTimeConstrained();
         advancedPeer.enableTimeConstrained();
         await(enabling, callbackModel, interactionName,
            () -> enablingCallbacks.timeConstrainedEnabled,
            "implicit-advance member Time Constrained callback");
         await(advancedPeer, callbackModel, interactionName,
            () -> peerCallbacks.timeConstrainedEnabled,
            "implicit-advance peer Time Constrained callback");

         producer.enableTimeRegulation(producerEpsilon);
         await(producer, callbackModel, interactionName,
            () -> producerCallbacks.timeRegulationEnabled != null,
            "implicit-advance producer Time Regulation callback");

         LogicalTime<?, ?> messageTime = offset(producerInitial, producerEpsilon, 5);
         LogicalTime<?, ?> producerTarget = offset(producerInitial, producerEpsilon, 10);
         LogicalTime<?, ?> peerTarget = offset(peerInitial, peerEpsilon, 10);
         LogicalTime<?, ?> expectedEnableTime = offset(enablingInitial, enablingEpsilon, 8);
         LogicalTimeInterval enablingLookahead =
            (LogicalTimeInterval) enablingEpsilon.add(enablingEpsilon);
         byte[] payload = {0x49, 0x41, 0x41};
         byte[] tag = {0x49, 0x41, 0x54};
         ParameterHandleValueMap parameters =
            producer.getParameterHandleValueMapFactory().create(1);
         parameters.put(producerParameter, payload.clone());
         MessageRetractionReturn retraction = producer.sendInteraction(
            producerClass, parameters, tag.clone(), messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "implicit-advance timestamped interaction returned an invalid retraction handle");
         JavaTckSupport.check(enablingCallbacks.interactions.isEmpty(),
            "queued TSO was delivered before the constrained member advanced");

         producer.timeAdvanceRequest(producerTarget);
         await(producer, callbackModel, interactionName,
            () -> producerCallbacks.grants.size() == 1,
            "implicit-advance producer Time Advance Grant");
         advancedPeer.timeAdvanceRequest(peerTarget);
         await(advancedPeer, callbackModel, interactionName,
            () -> peerCallbacks.grants.size() == 1,
            "implicit-advance peer Time Advance Grant");
         JavaTckSupport.check(enablingCallbacks.interactions.isEmpty(),
            "queued TSO was delivered before Time Regulation was enabled");

         enabling.enableTimeRegulation(enablingLookahead);
         await(enabling, callbackModel, interactionName,
            () -> enablingCallbacks.timeRegulationEnabled != null,
            "implicit-advance member Time Regulation callback");
         JavaTckSupport.check(expectedEnableTime.equals(
               enablingCallbacks.timeRegulationEnabled),
            "Enable Time Regulation did not assign the smallest feasible logical time");
         JavaTckSupport.check(enablingCallbacks.callbackOrder.equals(
               Arrays.asList("interaction", "regulation-enabled")),
            "queued TSO was not delivered before the Time Regulation Enabled callback: "
               + enablingCallbacks.callbackOrder);
         JavaTckSupport.check(enablingCallbacks.grants.isEmpty(),
            "implicit Time Regulation advance incorrectly produced a separate Time Advance Grant");
         JavaTckSupport.check(enablingCallbacks.interactions.size() == 1,
            "implicit Time Regulation advance did not deliver the queued TSO exactly once");

         Delivery delivery = enablingCallbacks.interactions.get(0);
         byte[] receivedPayload = delivery.parameters.get(enablingParameter);
         JavaTckSupport.check(enablingClass.equals(delivery.interactionClass)
               && producerHandle.equals(delivery.producer)
               && messageTime.equals(delivery.time)
               && delivery.sentOrder == OrderType.TIMESTAMP
               && delivery.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(delivery.retraction)
               && Arrays.equals(tag, delivery.tag)
               && delivery.parameters.size() == 1
               && Arrays.equals(payload, receivedPayload),
            "implicit Time Regulation advance did not preserve the queued TSO callback data");
         JavaTckSupport.check(expectedEnableTime.equals(enabling.queryLogicalTime()),
            "queried logical time did not match the implicit Time Regulation advance");

         enabling.unsubscribeInteractionClass(enablingClass);
         subscribed = false;
         producer.unpublishInteractionClass(producerClass);
         published = false;
         advancedPeer.resignFederationExecution(ResignAction.NO_ACTION);
         peerJoined = false;
         enabling.resignFederationExecution(ResignAction.NO_ACTION);
         enablingJoined = false;
         producer.resignFederationExecution(ResignAction.NO_ACTION);
         producerJoined = false;
         producer.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (subscribed && enablingJoined && enablingClass != null) {
            try { enabling.unsubscribeInteractionClass(enablingClass); }
            catch (Exception ignored) { }
         }
         if (published && producerJoined && producerClass != null) {
            try { producer.unpublishInteractionClass(producerClass); }
            catch (Exception ignored) { }
         }
         if (peerJoined) {
            try { advancedPeer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (enablingJoined) {
            try { enabling.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (producerJoined) {
            try { producer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (federationCreated) {
            try { producer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (peerConnected) {
            try { advancedPeer.disconnect(); }
            catch (Exception ignored) { }
         }
         if (enablingConnected) {
            try { enabling.disconnect(); }
            catch (Exception ignored) { }
         }
         if (producerConnected) {
            try { producer.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String interactionName, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 0.0);
         } else {
            ambassador.getInteractionClassHandle(interactionName);
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Delivery {
      private final InteractionClassHandle interactionClass;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Delivery(Object[] arguments) {
         interactionClass = (InteractionClassHandle) arguments[0];
         parameters = (ParameterHandleValueMap) arguments[1];
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile boolean timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeRegulationEnabled;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<Delivery> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add("regulation-enabled");
               } else if ("timeAdvanceGrant".equals(name)) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  interactions.add(new Delivery(arguments));
                  callbackOrder.add("interaction");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK implicit Time Regulation advance callbacks";
                  }
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
