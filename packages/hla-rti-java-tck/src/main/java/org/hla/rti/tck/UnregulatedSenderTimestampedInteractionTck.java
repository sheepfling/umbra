package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TimeQueryReturn;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks timestamp pass-through from a joined sender that is not time regulating. */
final class UnregulatedSenderTimestampedInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private UnregulatedSenderTimestampedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      Recorder regulatorCallbacks = new Recorder();
      String federation = "java-tck-unregulated-timestamped-interaction-" + UUID.randomUUID();
      boolean senderConnected = false, receiverConnected = false, regulatorConnected = false;
      boolean federationCreated = false;
      boolean senderJoined = false, receiverJoined = false, regulatorJoined = false;
      boolean published = false, subscribed = false;
      InteractionClassHandle senderClass = null, receiverClass = null;
      try {
         sender.connect(senderCallbacks.proxy(), callbackModel);
         senderConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         regulatorConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         sender.joinFederationExecution("java-tck-unregulated-interaction-sender", federation);
         senderJoined = true;
         receiver.joinFederationExecution("java-tck-unregulated-interaction-receiver", federation);
         receiverJoined = true;
         regulator.joinFederationExecution("java-tck-unregulated-interaction-regulator", federation);
         regulatorJoined = true;

         senderClass = sender.getInteractionClassHandle(interactionName);
         receiverClass = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle senderParameter = sender.getParameterHandle(senderClass, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverClass, parameterName);
         JavaTckSupport.check(senderClass != null && receiverClass != null
               && senderParameter != null && receiverParameter != null,
            "adapter interaction FOM did not resolve the standard handles");
         sender.publishInteractionClass(senderClass);
         published = true;
         sender.changeInteractionOrderType(senderClass, hla.rti1516_2025.OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);
         subscribed = true;

         LogicalTimeFactory<?, ?> senderFactory = sender.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         JavaTckSupport.check(senderFactory.getName().equals(receiverFactory.getName())
               && senderFactory.getName().equals(regulatorFactory.getName()),
            "unregulated interaction federation selected different logical-time factories");
         LogicalTimeInterval lookahead = (LogicalTimeInterval) regulatorFactory.makeEpsilon();
         LogicalTime<?, ?> senderInitial = sender.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         JavaTckSupport.check(senderInitial.equals(receiverInitial)
               && senderInitial.equals(regulatorInitial),
            "joined federates did not start at the same logical time");

         receiver.enableTimeConstrained();
         await(receiver, callbackModel, interactionName,
            () -> receiverCallbacks.timeConstrainedEnabled,
            "time-constrained role callback");
         regulator.enableTimeRegulation(lookahead);
         await(regulator, callbackModel, interactionName,
            () -> regulatorCallbacks.timeRegulationEnabled,
            "time-regulation role callback");

         LogicalTime<?, ?> expectedGalt = offset(regulatorInitial, lookahead, 1);
         TimeQueryReturn galt = sender.queryGALT();
         JavaTckSupport.check(galt.timeIsValid && expectedGalt.equals(galt.time),
            "unregulated sender did not observe the separate regulator's defined GALT");
         JavaTckSupport.check(receiverInitial.equals(receiver.queryLogicalTime()),
            "receiver logical time changed before its time advance request");

         LogicalTime<?, ?> timestamp = offset(senderInitial, lookahead, 2);
         LogicalTime<?, ?> receiverTarget = offset(receiverInitial, lookahead, 2);
         LogicalTime<?, ?> firstTimeAfterGalt = offset(expectedGalt, lookahead, 1);
         JavaTckSupport.check(timestamp.equals(receiverTarget),
            "timestamp did not match the receiver's requested logical time");
         JavaTckSupport.check(timestamp.equals(firstTimeAfterGalt),
            "timestamp was not later than the sender's defined GALT");

         byte[] payload = {0x55, 0x4E, 0x52};
         byte[] tag = {0x54, 0x53, 0x4F};
         ParameterHandleValueMap parameters = sender.getParameterHandleValueMapFactory().create(1);
         parameters.put(senderParameter, payload.clone());
         sender.sendInteraction(senderClass, parameters, tag.clone(), timestamp);
         receiver.timeAdvanceRequest(receiverTarget);

         awaitBoth(regulator, receiver, callbackModel, interactionName,
            () -> receiverCallbacks.interactions.size() == 1,
            "timestamped interaction delivery");

         Delivery delivery = receiverCallbacks.interactions.get(0);
         JavaTckSupport.check(receiverCallbacks.interactions.size() == 1
               && receiverClass.equals(delivery.interactionClass)
               && delivery.parameters.size() == 1
               && Arrays.equals(payload, delivery.parameters.get(receiverParameter))
               && Arrays.equals(tag, delivery.tag)
               && timestamp.equals(delivery.time),
            "timestamped interaction callback did not preserve the supplied payload and time");
         receiver.unsubscribeInteractionClass(receiverClass);
         subscribed = false;
         sender.unpublishInteractionClass(senderClass);
         published = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         sender.resignFederationExecution(ResignAction.NO_ACTION);
         senderJoined = false;
         sender.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (subscribed && receiverJoined && receiverClass != null) {
            try { receiver.unsubscribeInteractionClass(receiverClass); }
            catch (Exception ignored) { }
         }
         if (published && senderJoined && senderClass != null) {
            try { sender.unpublishInteractionClass(senderClass); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (regulatorJoined) {
            try { regulator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (senderJoined) {
            try { sender.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (federationCreated) {
            try { sender.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (regulatorConnected) {
            try { regulator.disconnect(); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (senderConnected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
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

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel model, String interactionName, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            first.evokeMultipleCallbacks(0.0, 0.0);
            second.evokeMultipleCallbacks(0.0, 0.0);
         } else {
            first.getInteractionClassHandle(interactionName);
            second.getInteractionClassHandle(interactionName);
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Delivery {
      private final InteractionClassHandle interactionClass;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final LogicalTime<?, ?> time;

      private Delivery(Object[] arguments) {
         interactionClass = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         time = (LogicalTime<?, ?>) arguments[6];
      }
   }

   private static final class Recorder {
      private volatile boolean timeConstrainedEnabled;
      private volatile boolean timeRegulationEnabled;
      private final List<Delivery> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  interactions.add(new Delivery(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK unregulated timestamped interaction callbacks";
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
