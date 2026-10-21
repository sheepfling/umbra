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

/** Checks an unconstrained FQR receiver's receive-order delivery before its grant. */
final class FqrNonTimeConstrainedTsoTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FqrNonTimeConstrainedTsoTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-fqr-unconstrained-tso-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-fqr-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-fqr-unconstrained", federation);
         receiverJoined = true;

         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction != null && receiverInteraction != null
               && publisherParameter != null && receiverParameter != null
               && publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            "Unconstrained FQR adapter FOM did not resolve stable interaction handles");
         publisher.publishInteractionClass(publisherInteraction);
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "Unconstrained FQR members selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "Unconstrained FQR members began at different logical times");
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         publisher.enableTimeRegulation(publisherEpsilon);
         await(publisher, callbackModel,
            () -> publisherCallbacks.regulationEnabled == 1,
            "unconstrained FQR publisher regulation callback");

         LogicalTime<?, ?> messageTime = offset(publisherInitial, publisherEpsilon, 7);
         LogicalTime<?, ?> requestTime = offset(receiverInitial, receiverEpsilon, 10);
         byte[] payload = {0x46, 0x51, 0x52, 0x2d, 0x4e, 0x54, 0x43};
         byte[] tag = {0x46, 0x51, 0x52};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload.clone());
         MessageRetractionReturn retraction = publisher.sendInteraction(
            publisherInteraction, values, tag.clone(), messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "Unconstrained FQR timestamped send returned no valid retraction handle");
         JavaTckSupport.check(publisher.queryLogicalTime().isInitial(),
            "Producer advanced before the unconstrained FQR request");

         // The receiver deliberately never enables Time Constrained.
         receiver.flushQueueRequest(requestTime);
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() >= 1
               && receiverCallbacks.flushGrantTimes.size() >= 1,
            "unconstrained FQR receive-order delivery and grant");
         JavaTckSupport.check(receiverCallbacks.interactions.size() == 1
               && receiverCallbacks.flushGrantTimes.size() == 1
               && receiverCallbacks.receiveOrderOverloadCount == 0
               && receiverCallbacks.callbackOrder.equals(Arrays.asList(
                  "interaction", "flush-grant")),
            "Unconstrained FQR grant preceded or duplicated receive-order delivery");
         InteractionRecord event = receiverCallbacks.interactions.get(0);
         JavaTckSupport.check(receiverInteraction.equals(event.interaction)
               && event.values.size() == 1
               && Arrays.equals(payload, event.values.get(receiverParameter))
               && Arrays.equals(tag, event.tag)
               && messageTime.equals(event.time)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == OrderType.RECEIVE,
            "Unconstrained FQR did not preserve timestamped input as receive-order delivery");
         JavaTckSupport.check(publisher.queryLogicalTime().isInitial(),
            "Unconstrained FQR waited for the regulating producer to advance");

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
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
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
         sentOrder = (OrderType) arguments[3];
         time = (LogicalTime<?, ?>) arguments[6];
         receivedOrder = (OrderType) arguments[8];
      }
   }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int receiveOrderOverloadCount;
      private final List<InteractionRecord> interactions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushGrantTimes = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  regulationEnabled++;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  receiveOrderOverloadCount++;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  InteractionRecord event = new InteractionRecord(arguments);
                  callbackOrder.add("interaction");
                  interactions.add(event);
               } else if ("flushQueueGrant".equals(name)
                     && arguments != null && arguments.length == 2) {
                  callbackOrder.add("flush-grant");
                  flushGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK unconstrained FQR callbacks";
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
