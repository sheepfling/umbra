package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
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

/** Checks a timestamped send at a zero-lookahead sender's just-granted time. */
final class TimestampedInteractionZeroLookaheadAtGrantTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedInteractionZeroLookaheadAtGrantTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-zero-lookahead-send-at-grant-" + UUID.randomUUID();
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
         publisher.joinFederationExecution("java-tck-zero-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-zero-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publishedClass =
            publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle subscribedClass =
            receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publishedParameter = publisher.getParameterHandle(
            publishedClass, parameterName);
         ParameterHandle subscribedParameter = receiver.getParameterHandle(
            subscribedClass, parameterName);
         JavaTckSupport.check(publishedClass != null && subscribedClass != null
               && publishedParameter != null && subscribedParameter != null
               && publishedClass.equals(subscribedClass)
               && publishedParameter.equals(subscribedParameter),
            "Zero-lookahead adapter FOM did not resolve stable interaction handles");
         publisher.publishInteractionClass(publishedClass);
         publisher.changeInteractionOrderType(publishedClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(subscribedClass);

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "Zero-lookahead members selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "Zero-lookahead members began at different logical times");
         LogicalTimeInterval zero = (LogicalTimeInterval) publisherFactory.makeZero();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.constrainedEnabled == 1,
            "zero-lookahead receiver Time Constrained callback");
         publisher.enableTimeRegulation(zero);
         await(publisher, callbackModel,
            () -> publisherCallbacks.regulationEnabled == 1,
            "zero-lookahead publisher Time Regulation callback");
         JavaTckSupport.check(zero.equals(publisher.queryLookahead()),
            "Regulating sender did not retain zero actual lookahead");

         LogicalTime<?, ?> grantedTime = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime<?, ?> nextTime = offset(publisherInitial, publisherEpsilon, 6);
         LogicalTime<?, ?> receiverTarget = offset(receiverInitial, receiverEpsilon, 5);
         JavaTckSupport.check(grantedTime.equals(receiverTarget),
            "Zero-lookahead members calculated different target times");
         publisher.timeAdvanceRequest(grantedTime);
         await(publisher, callbackModel,
            () -> publisherCallbacks.grants.size() == 1,
            "zero-lookahead publisher grant at time five");
         JavaTckSupport.check(grantedTime.equals(publisherCallbacks.grants.get(0))
               && grantedTime.equals(publisher.queryLogicalTime()),
            "Zero-lookahead publisher did not reach the requested time");

         byte[] payload = {(byte) 0xb5, 0x05};
         byte[] tag = {(byte) 0xb5, 0x06};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publishedParameter, payload.clone());
         MessageRetractionReturn retraction = publisher.sendInteraction(
            publishedClass, values, tag.clone(), grantedTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "Zero-lookahead post-grant send at the granted time was rejected");
         JavaTckSupport.check(receiverCallbacks.interactions.isEmpty(),
            "Zero-lookahead TSO arrived before the receiver requested its time");

         publisher.timeAdvanceRequest(nextTime);
         await(publisher, callbackModel,
            () -> publisherCallbacks.grants.size() == 2,
            "zero-lookahead publisher forward grant");
         receiver.timeAdvanceRequest(receiverTarget);
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() == 1
               && receiverCallbacks.grants.size() == 1,
            "zero-lookahead receiver matching grant");
         InteractionRecord event = receiverCallbacks.interactions.get(0);
         JavaTckSupport.check(receiverCallbacks.receiveOrderOverloadCount == 0
               && receiverCallbacks.callbackOrder.equals(Arrays.asList("interaction", "grant"))
               && subscribedClass.equals(event.interaction)
               && event.values.size() == 1
               && Arrays.equals(payload, event.values.get(subscribedParameter))
               && Arrays.equals(tag, event.tag)
               && grantedTime.equals(event.time)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(event.retraction)
               && receiverTarget.equals(receiverCallbacks.grants.get(0)),
            "Zero-lookahead post-grant callback changed its metadata or grant order");

         receiver.disableTimeConstrained();
         publisher.disableTimeRegulation();
         receiver.unsubscribeInteractionClass(subscribedClass);
         publisher.unpublishInteractionClass(publishedClass);
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
      private final MessageRetractionHandle retraction;

      private InteractionRecord(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         values = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         sentOrder = (OrderType) arguments[3];
         time = (LogicalTime<?, ?>) arguments[6];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile int constrainedEnabled;
      private volatile int regulationEnabled;
      private volatile int receiveOrderOverloadCount;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<InteractionRecord> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  constrainedEnabled++;
               } else if ("timeRegulationEnabled".equals(name)) {
                  regulationEnabled++;
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
                  callbackOrder.add("interaction");
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK zero-lookahead callbacks";
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
