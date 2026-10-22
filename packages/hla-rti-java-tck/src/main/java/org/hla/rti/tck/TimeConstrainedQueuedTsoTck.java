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

/** Checks that a TSO interaction stays queued across pending Time Constrained enablement. */
final class TimeConstrainedQueuedTsoTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeConstrainedQueuedTsoTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder regulatorCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-time-constrained-queued-tso-" + UUID.randomUUID();
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
         regulator.joinFederationExecution("java-tck-queued-regulator", federation);
         regulatorJoined = true;
         receiver.joinFederationExecution("java-tck-queued-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle regulatorInteraction =
            regulator.getInteractionClassHandle(interactionName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionName);
         ParameterHandle parameter = regulator.getParameterHandle(
            regulatorInteraction, parameterName);
         JavaTckSupport.check(regulatorInteraction != null && receiverInteraction != null
               && regulatorInteraction.equals(receiverInteraction) && parameter != null,
            "Time Constrained queued-TSO adapter FOM did not resolve interaction handles");
         regulator.publishInteractionClass(regulatorInteraction);
         regulator.changeInteractionOrderType(regulatorInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);

         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(regulatorFactory.getName().equals(receiverFactory.getName()),
            "Time Constrained queued-TSO members selected different time factories");
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(regulatorInitial.equals(receiverInitial),
            "Time Constrained queued-TSO members began at different times");
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         LogicalTime<?, ?> receiverFive = offset(receiverInitial, receiverEpsilon, 5);
         LogicalTime<?, ?> regulatorFour = offset(regulatorInitial, regulatorEpsilon, 4);
         LogicalTime<?, ?> regulatorFive = offset(regulatorInitial, regulatorEpsilon, 5);
         LogicalTime<?, ?> regulatorSix = offset(regulatorInitial, regulatorEpsilon, 6);
         LogicalTime<?, ?> receiverSix = offset(receiverInitial, receiverEpsilon, 6);

         regulator.enableTimeRegulation(regulatorEpsilon);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.regulationEnabled == 1,
            "Time Constrained queued-TSO regulator enablement");
         receiver.timeAdvanceRequest(receiverFive);
         await(receiver, callbackModel,
            () -> receiverCallbacks.grants.size() == 1,
            "Time Constrained queued-TSO unconstrained grant");
         JavaTckSupport.check(receiverFive.equals(receiverCallbacks.grants.get(0)),
            "Time Constrained queued-TSO unconstrained receiver did not reach time five");

         receiver.enableTimeConstrained();
         serviceOnce(receiver, callbackModel);
         JavaTckSupport.check(receiverCallbacks.constrainedEnabled == 0,
            "Time Constrained enabled before the regulator reached the receiver time");

         byte[] payload = {0x54, 0x53, 0x4f};
         byte[] tag = {0x54, 0x43, 0x45, 0x2d, 0x54, 0x53, 0x4f};
         ParameterHandleValueMap values =
            regulator.getParameterHandleValueMapFactory().create(1);
         values.put(parameter, payload.clone());
         MessageRetractionReturn retraction = regulator.sendInteraction(
            regulatorInteraction, values, tag.clone(), regulatorSix);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "Time Constrained queued-TSO send returned no valid retraction handle");
         serviceOnce(receiver, callbackModel);
         checkNoDelivery(receiverCallbacks, "while enablement was pending");

         regulator.timeAdvanceRequest(regulatorFour);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.grants.size() == 1,
            "Time Constrained queued-TSO regulator grant to four");
         serviceOnce(receiver, callbackModel);
         JavaTckSupport.check(receiverCallbacks.constrainedEnabled == 0,
            "Time Constrained enabled while the regulator frontier was at receiver time");
         checkNoDelivery(receiverCallbacks, "while GALT remained at receiver time");

         regulator.timeAdvanceRequest(regulatorFive);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.grants.size() == 2,
            "Time Constrained queued-TSO regulator grant to five");
         await(receiver, callbackModel,
            () -> receiverCallbacks.constrainedEnabled == 1,
            "Time Constrained queued-TSO transition callback");
         JavaTckSupport.check(receiverFive.equals(receiverCallbacks.constrainedTime),
            "Time Constrained callback did not report the receiver's current time");
         checkNoDelivery(receiverCallbacks, "after enablement but before TAR(6)");
         int orderBeforeAdvance = receiverCallbacks.callbackOrder.size();

         regulator.timeAdvanceRequest(regulatorSix);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.grants.size() == 3,
            "Time Constrained queued-TSO regulator grant to six");
         JavaTckSupport.check(regulatorSix.equals(regulatorCallbacks.grants.get(2)),
            "Time Constrained queued-TSO regulator did not reach time six");

         receiver.timeAdvanceRequest(receiverSix);
         await(receiver, callbackModel,
            () -> receiverCallbacks.timedInteractions.size() == 1
               && receiverCallbacks.grants.size() == 2,
            "Time Constrained queued-TSO delivery and grant at six");
         InteractionRecord received = receiverCallbacks.timedInteractions.get(0);
         JavaTckSupport.check(receiverCallbacks.receiveOrderInteractions == 0
               && receiverInteraction.equals(received.interaction)
               && Arrays.equals(payload, received.values.get(parameter))
               && Arrays.equals(tag, received.tag)
               && receiverSix.equals(received.time)
               && received.sentOrder == OrderType.TIMESTAMP
               && received.receivedOrder == OrderType.TIMESTAMP,
            "Queued interaction did not retain timestamp-order delivery across enablement");
         JavaTckSupport.check(receiverSix.equals(receiverCallbacks.grants.get(1))
               && receiverCallbacks.callbackOrder.size() == orderBeforeAdvance + 2
               && "interaction".equals(receiverCallbacks.callbackOrder.get(orderBeforeAdvance))
               && "grant".equals(receiverCallbacks.callbackOrder.get(orderBeforeAdvance + 1)),
            "Queued timestamped interaction did not precede TAR(6)'s grant");

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

   private static void checkNoDelivery(Recorder recorder, String phase) {
      JavaTckSupport.check(recorder.receiveOrderInteractions == 0
            && recorder.timedInteractions.isEmpty(),
         "Time Constrained queued-TSO interaction escaped " + phase);
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
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
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
      private volatile int receiveOrderInteractions;
      private volatile LogicalTime<?, ?> constrainedTime;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<InteractionRecord> timedInteractions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

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
                  callbackOrder.add("time-constrained-enabled");
                  constrainedEnabled++;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  callbackOrder.add("grant");
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  receiveOrderInteractions++;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  callbackOrder.add("interaction");
                  timedInteractions.add(new InteractionRecord(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK queued-TSO callbacks";
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
