package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
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

/** Checks receive-order delivery before and timestamp-order delivery after Time Constrained. */
final class TimeConstrainedReceiveOrderBoundaryTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeConstrainedReceiveOrderBoundaryTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder regulatorCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-time-constrained-receive-boundary-"
         + UUID.randomUUID();
      boolean regulatorConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean regulatorJoined = false;
      boolean receiverJoined = false;
      try {
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         regulatorConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         regulator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         regulator.joinFederationExecution("java-tck-receive-regulator", federation);
         regulatorJoined = true;
         receiver.joinFederationExecution("java-tck-receive-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle regulatorInteraction =
            regulator.getInteractionClassHandle(interactionName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionName);
         ParameterHandle parameter = regulator.getParameterHandle(
            regulatorInteraction, parameterName);
         JavaTckSupport.check(regulatorInteraction != null && receiverInteraction != null
               && regulatorInteraction.equals(receiverInteraction) && parameter != null,
            "Time Constrained receive-order adapter FOM did not resolve interaction handles");
         regulator.publishInteractionClass(regulatorInteraction);
         regulator.changeInteractionOrderType(regulatorInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);

         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(regulatorFactory.getName().equals(receiverFactory.getName()),
            "Time Constrained receive-order members selected different time factories");
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(regulatorInitial.equals(receiverInitial),
            "Time Constrained receive-order members began at different times");
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         LogicalTime<?, ?> regulatorFive = offset(regulatorInitial, regulatorEpsilon, 5);
         LogicalTime<?, ?> receiverFive = offset(receiverInitial, receiverEpsilon, 5);
         LogicalTime<?, ?> regulatorSix = offset(regulatorInitial, regulatorEpsilon, 6);
         LogicalTime<?, ?> regulatorSeven = offset(regulatorInitial, regulatorEpsilon, 7);
         LogicalTime<?, ?> regulatorEight = offset(regulatorInitial, regulatorEpsilon, 8);
         LogicalTime<?, ?> receiverEight = offset(receiverInitial, receiverEpsilon, 8);
         JavaTckSupport.check(regulatorFive.equals(receiverFive)
               && regulatorEight.equals(receiverEight),
            "Time Constrained receive-order members use different logical-time values");

         regulator.enableTimeRegulation(regulatorEpsilon);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.regulationEnabled == 1,
            "Time Constrained receive-order regulator enablement");
         byte[] payload = {0x54, 0x43, 0x45};
         byte[] futureTag = {0x52, 0x4f, 0x2d, 0x46};
         byte[] equalTimeTag = {0x52, 0x4f, 0x2d, 0x45};
         byte[] postTransitionTag = {0x52, 0x4f, 0x2d, 0x54};
         ParameterHandleValueMap values =
            regulator.getParameterHandleValueMapFactory().create(1);
         values.put(parameter, payload.clone());

         checkRetraction(regulator.sendInteraction(regulatorInteraction, values,
            futureTag.clone(), regulatorFive), "future pre-transition interaction");
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() == 1,
            "future pre-transition receive-order interaction");
         checkInteraction(receiverCallbacks.interactions.get(0), receiverInteraction,
            parameter, payload, futureTag, receiverFive, OrderType.RECEIVE);

         receiver.timeAdvanceRequest(receiverFive);
         await(receiver, callbackModel,
            () -> receiverCallbacks.grants.size() == 1,
            "unconstrained receiver grant to five");
         JavaTckSupport.check(receiverFive.equals(receiverCallbacks.grants.get(0)),
            "Unconstrained receiver did not reach the transition timestamp");

         checkRetraction(regulator.sendInteraction(regulatorInteraction, values,
            equalTimeTag.clone(), regulatorFive), "equal-time pre-transition interaction");
         receiver.enableTimeConstrained();
         serviceOnce(receiver, callbackModel);
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() == 2,
            "equal-time pre-transition receive-order interaction");
         JavaTckSupport.check(receiverCallbacks.constrainedEnabled == 0,
            "Time Constrained Enabled preceded the regulator's safe frontier");
         checkInteraction(receiverCallbacks.interactions.get(1), receiverInteraction,
            parameter, payload, equalTimeTag, receiverFive, OrderType.RECEIVE);

         regulator.timeAdvanceRequest(regulatorFive);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.grants.size() == 1,
            "regulator grant to five");
         await(receiver, callbackModel,
            () -> receiverCallbacks.constrainedEnabled == 1,
            "Time Constrained receive-order transition callback");
         JavaTckSupport.check(receiverFive.equals(receiverCallbacks.constrainedTime)
               && receiverCallbacks.transitionOrder.equals(Arrays.asList(
                  "interaction", "interaction", "time-constrained-enabled")),
            "Pre-transition receive-order callbacks did not precede Time Constrained Enabled");

         checkRetraction(regulator.sendInteraction(regulatorInteraction, values,
            postTransitionTag.clone(), regulatorSeven),
            "post-transition timestamped interaction");
         serviceOnce(receiver, callbackModel);
         JavaTckSupport.check(receiverCallbacks.interactions.size() == 2,
            "Post-transition timestamped interaction escaped before receiver advancement");
         int orderBeforeAdvance = receiverCallbacks.callbackOrder.size();
         receiver.timeAdvanceRequest(receiverEight);
         JavaTckSupport.check(receiverCallbacks.grants.size() == 1,
            "Constrained receiver granted TAR(8) before regulator advancement");

         regulator.timeAdvanceRequest(regulatorSix);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.grants.size() == 2,
            "regulator grant to six");
         JavaTckSupport.check(receiverCallbacks.grants.size() == 1,
            "Receiver granted TAR(8) while regulator frontier was only seven");
         regulator.timeAdvanceRequest(regulatorEight);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.grants.size() == 3,
            "regulator grant to eight");
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() == 3
               && receiverCallbacks.grants.size() == 2,
            "post-transition timestamped interaction before TAR(8) grant");
         checkInteraction(receiverCallbacks.interactions.get(2), receiverInteraction,
            parameter, payload, postTransitionTag, regulatorSeven, OrderType.TIMESTAMP);
         JavaTckSupport.check(receiverCallbacks.receiveOrderOverloadCount == 0
               && receiverCallbacks.callbackOrder.size() == orderBeforeAdvance + 2
               && "interaction".equals(receiverCallbacks.callbackOrder.get(orderBeforeAdvance))
               && "grant".equals(receiverCallbacks.callbackOrder.get(orderBeforeAdvance + 1))
               && receiverCallbacks.transitionOrder.equals(Arrays.asList(
                  "interaction", "interaction", "time-constrained-enabled", "interaction"))
               && receiverEight.equals(receiverCallbacks.grants.get(1)),
            "Post-transition TSO did not precede TAR(8)'s grant");

         receiver.disableTimeConstrained();
         regulator.disableTimeRegulation();
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         regulator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (regulatorJoined) resign(regulator);
         if (created) {
            try { regulator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (regulatorConnected) disconnect(regulator);
      }
   }

   private static void checkRetraction(MessageRetractionReturn result, String phase) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null, phase + " returned no valid retraction handle");
   }

   private static void checkInteraction(InteractionRecord event,
         InteractionClassHandle interaction, ParameterHandle parameter,
         byte[] payload, byte[] tag, LogicalTime<?, ?> time, OrderType receivedOrder) {
      JavaTckSupport.check(interaction.equals(event.interaction)
            && event.values.size() == 1
            && Arrays.equals(payload, event.values.get(parameter))
            && Arrays.equals(tag, event.tag)
            && time.equals(event.time)
            && event.sentOrder == OrderType.TIMESTAMP
            && event.receivedOrder == receivedOrder,
         "Time Constrained boundary callback changed its identity, timestamp, or order");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void serviceOnce(RTIambassador ambassador, CallbackModel model)
         throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
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

   private static final class InteractionRecord {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap values;
      private final byte[] tag;
      private final OrderType sentOrder;
      private final LogicalTime<?, ?> time;
      private final OrderType receivedOrder;

      private InteractionRecord(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         values = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         sentOrder = (OrderType) arguments[7];
         time = (LogicalTime<?, ?>) arguments[6];
         receivedOrder = (OrderType) arguments[8];
      }
   }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private volatile int receiveOrderOverloadCount;
      private volatile LogicalTime<?, ?> constrainedTime;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<InteractionRecord> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final List<String> transitionOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  regulationEnabled++;
               } else if ("timeConstrainedEnabled".equals(name)
                     && arguments != null && arguments.length == 1) {
                  constrainedTime = (LogicalTime<?, ?>) arguments[0];
                  transitionOrder.add("time-constrained-enabled");
                  constrainedEnabled++;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  callbackOrder.add("grant");
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  receiveOrderOverloadCount++;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  InteractionRecord event = new InteractionRecord(arguments);
                  transitionOrder.add("interaction");
                  callbackOrder.add("interaction");
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK receive-order boundary callbacks";
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
