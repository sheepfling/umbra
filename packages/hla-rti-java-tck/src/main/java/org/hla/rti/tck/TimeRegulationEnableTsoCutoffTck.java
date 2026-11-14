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

/** Checks the assigned-time cutoff for queued timestamped interactions. */
final class TimeRegulationEnableTsoCutoffTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeRegulationEnableTsoCutoffTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String federateType, String interactionClassName, String parameterName,
         CallbackModel callbackModel)
         throws Exception {
      RTIambassador producer = factory.getRtiAmbassador();
      RTIambassador enabling = factory.getRtiAmbassador();
      RTIambassador advancedPeer = factory.getRtiAmbassador();
      Recorder producerCallbacks = new Recorder();
      Recorder enablingCallbacks = new Recorder();
      Recorder peerCallbacks = new Recorder();
      String federation = "java-tck-time-regulation-enable-tso-cutoff-" + UUID.randomUUID();
      boolean producerConnected = false;
      boolean enablingConnected = false;
      boolean peerConnected = false;
      boolean created = false;
      boolean producerJoined = false;
      boolean enablingJoined = false;
      boolean peerJoined = false;
      try {
         producer.connect(producerCallbacks.proxy(), callbackModel);
         producerConnected = true;
         enabling.connect(enablingCallbacks.proxy(), callbackModel);
         enablingConnected = true;
         advancedPeer.connect(peerCallbacks.proxy(), callbackModel);
         peerConnected = true;
         producer.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producerFederate = producer.joinFederationExecution(
            "java-tck-tso-cutoff-producer", federateType, federation);
         producerJoined = true;
         enabling.joinFederationExecution(
            "java-tck-tso-cutoff-enabling", federateType, federation);
         enablingJoined = true;
         advancedPeer.joinFederationExecution(
            "java-tck-tso-cutoff-peer", federateType, federation);
         peerJoined = true;

         InteractionClassHandle producerInteraction =
            producer.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle enablingInteraction =
            enabling.getInteractionClassHandle(interactionClassName);
         ParameterHandle producerParameter =
            producer.getParameterHandle(producerInteraction, parameterName);
         ParameterHandle enablingParameter =
            enabling.getParameterHandle(enablingInteraction, parameterName);
         JavaTckSupport.check(producerInteraction != null && enablingInteraction != null
               && producerParameter != null && enablingParameter != null
               && producerInteraction.equals(enablingInteraction)
               && producerParameter.equals(enablingParameter),
            "Time Regulation TSO cutoff could not resolve stable interaction handles");
         producer.publishInteractionClass(producerInteraction);
         producer.changeInteractionOrderType(producerInteraction, OrderType.TIMESTAMP);
         enabling.subscribeInteractionClass(enablingInteraction);

         LogicalTimeFactory<?, ?> producerFactory = producer.getTimeFactory();
         LogicalTimeFactory<?, ?> enablingFactory = enabling.getTimeFactory();
         LogicalTimeFactory<?, ?> peerFactory = advancedPeer.getTimeFactory();
         JavaTckSupport.check(producerFactory.getName().equals(enablingFactory.getName())
               && producerFactory.getName().equals(peerFactory.getName()),
            "Time Regulation TSO cutoff members selected different logical-time factories");
         LogicalTime<?, ?> producerInitial = producer.queryLogicalTime();
         LogicalTime<?, ?> enablingInitial = enabling.queryLogicalTime();
         LogicalTime<?, ?> peerInitial = advancedPeer.queryLogicalTime();
         JavaTckSupport.check(producerInitial.equals(enablingInitial)
               && producerInitial.equals(peerInitial),
            "Time Regulation TSO cutoff members began at different logical times");
         LogicalTimeInterval producerEpsilon =
            (LogicalTimeInterval) producerFactory.makeEpsilon();
         LogicalTimeInterval enablingEpsilon =
            (LogicalTimeInterval) enablingFactory.makeEpsilon();
         LogicalTimeInterval peerEpsilon = (LogicalTimeInterval) peerFactory.makeEpsilon();

         enabling.enableTimeConstrained();
         await(enabling, callbackModel,
            () -> enablingCallbacks.timeConstrainedEnabled == 1,
            "Time Regulation TSO cutoff receiver Time Constrained callback");
         advancedPeer.enableTimeConstrained();
         await(advancedPeer, callbackModel,
            () -> peerCallbacks.timeConstrainedEnabled == 1,
            "Time Regulation TSO cutoff peer Time Constrained callback");
         producer.enableTimeRegulation(producerEpsilon);
         await(producer, callbackModel,
            () -> producerCallbacks.timeRegulationTimes.size() == 1,
            "Time Regulation TSO cutoff producer regulation callback");

         LogicalTime<?, ?> suppressedTime = offset(producerInitial, producerEpsilon, 5);
         LogicalTime<?, ?> inclusiveTime = offset(producerInitial, producerEpsilon, 8);
         LogicalTime<?, ?> producerTarget = offset(producerInitial, producerEpsilon, 10);
         LogicalTime<?, ?> peerTarget = offset(peerInitial, peerEpsilon, 10);
         LogicalTime<?, ?> assignedTime = offset(enablingInitial, enablingEpsilon, 8);
         LogicalTimeInterval enablingLookahead =
            (LogicalTimeInterval) enablingFactory.makeZero();
         for (int index = 0; index < 2; ++index) {
            enablingLookahead = (LogicalTimeInterval) enablingLookahead.add(enablingEpsilon);
         }

         byte[] suppressedPayload = {0x54, 0x53, 0x4f, 0x35};
         byte[] inclusivePayload = {0x54, 0x53, 0x4f, 0x38};
         byte[] suppressedTag = {0x43, 0x55, 0x54, 0x35};
         byte[] inclusiveTag = {0x43, 0x55, 0x54, 0x38};
         ParameterHandleValueMap suppressedValues =
            producer.getParameterHandleValueMapFactory().create(1);
         suppressedValues.put(producerParameter, suppressedPayload.clone());
         ParameterHandleValueMap inclusiveValues =
            producer.getParameterHandleValueMapFactory().create(1);
         inclusiveValues.put(producerParameter, inclusivePayload.clone());
         MessageRetractionReturn suppressedSend = producer.sendInteraction(
            producerInteraction, suppressedValues, suppressedTag.clone(), suppressedTime);
         MessageRetractionReturn inclusiveSend = producer.sendInteraction(
            producerInteraction, inclusiveValues, inclusiveTag.clone(), inclusiveTime);
         JavaTckSupport.check(valid(suppressedSend) && valid(inclusiveSend),
            "Time Regulation TSO cutoff timestamped sends returned invalid retraction handles");
         JavaTckSupport.drain(enabling);
         JavaTckSupport.check(enablingCallbacks.receivedInteractions.isEmpty(),
            "Time Regulation TSO cutoff delivered a queued interaction before the federates advanced");

         producer.timeAdvanceRequest(producerTarget);
         await(producer, callbackModel,
            () -> producerCallbacks.grants.size() == 1,
            "Time Regulation TSO cutoff producer Time Advance Grant");
         advancedPeer.timeAdvanceRequest(peerTarget);
         await(advancedPeer, callbackModel,
            () -> peerCallbacks.grants.size() == 1,
            "Time Regulation TSO cutoff peer Time Advance Grant");
         JavaTckSupport.check(producerTarget.equals(producerCallbacks.grants.get(0))
               && peerTarget.equals(peerCallbacks.grants.get(0)),
            "Time Regulation TSO cutoff producer or peer reached the wrong time");
         JavaTckSupport.drain(enabling);
         JavaTckSupport.check(enablingCallbacks.receivedInteractions.isEmpty(),
            "Time Regulation TSO cutoff delivered a queued interaction before regulation was enabled");

         enabling.enableTimeRegulation(enablingLookahead);
         await(enabling, callbackModel,
            () -> enablingCallbacks.timeRegulationTimes.size() == 1,
            "Time Regulation TSO cutoff receiver regulation callback");
         JavaTckSupport.check(enablingCallbacks.timeRegulationTimes.size() == 1
               && assignedTime.equals(enablingCallbacks.timeRegulationTimes.get(0)),
            "Time Regulation Enabled reported the wrong assigned federation time: expected "
               + assignedTime + " but received "
               + (enablingCallbacks.timeRegulationTimes.isEmpty() ? "no callback time"
                  : enablingCallbacks.timeRegulationTimes.get(0)));
         JavaTckSupport.check(enablingCallbacks.callbackOrder.equals(
               Arrays.asList("interaction", "regulation-enabled")),
            "the inclusive-boundary TSO was not delivered before Time Regulation Enabled");
         JavaTckSupport.check(enablingCallbacks.grants.isEmpty()
               && enablingCallbacks.receiveOrderInteractionCount == 0
               && enablingCallbacks.receivedInteractions.size() == 1,
            "Time Regulation enablement delivered a below-boundary TSO or an extra callback");

         ReceivedInteraction delivery = enablingCallbacks.receivedInteractions.get(0);
         JavaTckSupport.check(delivery.interaction.equals(enablingInteraction)
               && delivery.producer.equals(producerFederate)
               && delivery.parameters.size() == 1
               && delivery.parameters.containsKey(enablingParameter)
               && Arrays.equals(inclusivePayload,
                  delivery.parameters.get(enablingParameter))
               && delivery.time.equals(inclusiveTime)
               && delivery.sentOrder == OrderType.TIMESTAMP
               && delivery.receivedOrder == OrderType.TIMESTAMP
               && delivery.retraction != null
               && Arrays.equals(inclusiveTag, delivery.tag),
            "Time Regulation TSO cutoff did not preserve the inclusive-boundary callback");
         JavaTckSupport.check(assignedTime.equals(enabling.queryLogicalTime()),
            "Time Regulation TSO cutoff query returned the wrong current logical time");

         advancedPeer.resignFederationExecution(ResignAction.NO_ACTION);
         peerJoined = false;
         enabling.resignFederationExecution(ResignAction.NO_ACTION);
         enablingJoined = false;
         producer.resignFederationExecution(ResignAction.NO_ACTION);
         producerJoined = false;
         producer.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (peerJoined) resign(advancedPeer);
         if (enablingJoined) resign(enabling);
         if (producerJoined) resign(producer);
         if (created) {
            try { producer.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (peerConnected) disconnect(advancedPeer);
         if (enablingConnected) disconnect(enabling);
         if (producerConnected) disconnect(producer);
      }
   }

   private static boolean valid(MessageRetractionReturn result) {
      return result != null && result.retractionHandleIsValid && result.handle != null;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
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
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private ReceivedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile int timeConstrainedEnabled;
      private volatile int receiveOrderInteractionCount;
      private final List<LogicalTime<?, ?>> timeRegulationTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<ReceivedInteraction> receivedInteractions =
         new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("timeRegulationEnabled".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeRegulationTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("regulation-enabled");
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  receiveOrderInteractionCount++;
                  callbackOrder.add("receive-order-interaction");
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  receivedInteractions.add(new ReceivedInteraction(arguments));
                  callbackOrder.add("interaction");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK TSO cutoff callbacks";
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
