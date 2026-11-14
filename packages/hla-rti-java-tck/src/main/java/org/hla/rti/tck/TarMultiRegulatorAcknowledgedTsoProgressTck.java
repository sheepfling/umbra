package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
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

/** Checks safe TAR progress while two regulators have pending TSOs. */
final class TarMultiRegulatorAcknowledgedTsoProgressTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;

   private TarMultiRegulatorAcknowledgedTsoProgressTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador firstRegulator = factory.getRtiAmbassador();
      RTIambassador secondRegulator = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder firstEvents = new Recorder();
      Recorder secondEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-tar-multi-regulator-" + UUID.randomUUID();
      boolean firstConnected = false;
      boolean secondConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean firstJoined = false;
      boolean secondJoined = false;
      boolean receiverJoined = false;
      boolean firstPublished = false;
      boolean secondPublished = false;
      boolean subscribed = false;
      boolean receiverConstrained = false;
      boolean firstRegulating = false;
      boolean secondRegulating = false;
      InteractionClassHandle firstInteraction = null;
      InteractionClassHandle secondInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      try {
         firstRegulator.connect(firstEvents.proxy(), callbackModel);
         firstConnected = true;
         firstRegulator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle firstProducer = firstRegulator.joinFederationExecution(
            "java-tck-tar-multi-first-regulator", federation);
         firstJoined = true;

         secondRegulator.connect(secondEvents.proxy(), callbackModel);
         secondConnected = true;
         FederateHandle secondProducer = secondRegulator.joinFederationExecution(
            "java-tck-tar-multi-second-regulator", federation);
         secondJoined = true;

         receiver.connect(receiverEvents.proxy(), callbackModel);
         receiverConnected = true;
         receiver.joinFederationExecution("java-tck-tar-multi-receiver", federation);
         receiverJoined = true;

         firstInteraction = firstRegulator.getInteractionClassHandle(interactionName);
         secondInteraction = secondRegulator.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle firstParameter = firstRegulator.getParameterHandle(
            firstInteraction, parameterName);
         ParameterHandle secondParameter = secondRegulator.getParameterHandle(
            secondInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(firstInteraction != null && secondInteraction != null
               && receiverInteraction != null && firstParameter != null
               && secondParameter != null && receiverParameter != null
               && firstInteraction.equals(secondInteraction)
               && firstInteraction.equals(receiverInteraction)
               && firstParameter.equals(secondParameter)
               && firstParameter.equals(receiverParameter),
            "multi-regulator TAR lookup returned inconsistent handles");

         firstRegulator.publishInteractionClass(firstInteraction);
         firstPublished = true;
         firstRegulator.changeInteractionOrderType(firstInteraction, OrderType.TIMESTAMP);
         secondRegulator.publishInteractionClass(secondInteraction);
         secondPublished = true;
         secondRegulator.changeInteractionOrderType(secondInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> firstFactory = firstRegulator.getTimeFactory();
         LogicalTimeFactory<?, ?> secondFactory = secondRegulator.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(firstFactory.getName().equals(secondFactory.getName())
               && firstFactory.getName().equals(receiverFactory.getName()),
            "multi-regulator TAR members selected different logical-time factories");
         LogicalTimeInterval firstEpsilon =
            (LogicalTimeInterval) firstFactory.makeEpsilon();
         LogicalTimeInterval secondEpsilon =
            (LogicalTimeInterval) secondFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         LogicalTime firstInitial = firstRegulator.queryLogicalTime();
         LogicalTime secondInitial = secondRegulator.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(firstInitial.equals(secondInitial)
               && firstInitial.equals(receiverInitial),
            "multi-regulator TAR members began at different logical times");

         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(receiver, callbackModel,
            () -> receiverEvents.constrainedEnabled == 1,
            "multi-regulator receiver constrained callback");
         firstRegulator.enableTimeRegulation(firstEpsilon);
         firstRegulating = true;
         await(firstRegulator, callbackModel,
            () -> firstEvents.regulationEnabled == 1,
            "first regulator time-regulation callback");
         secondRegulator.enableTimeRegulation(secondEpsilon);
         secondRegulating = true;
         await(secondRegulator, callbackModel,
            () -> secondEvents.regulationEnabled == 1,
            "second regulator time-regulation callback");

         LogicalTime firstMessageTime = offset(firstInitial, firstEpsilon, 3);
         LogicalTime secondMessageTime = offset(secondInitial, secondEpsilon, 5);
         LogicalTime firstRegulatorTarget = offset(firstInitial, firstEpsilon, 7);
         LogicalTime secondRegulatorTarget = offset(secondInitial, secondEpsilon, 8);
         LogicalTime receiverFirstTarget = offset(receiverInitial, receiverEpsilon, 4);
         LogicalTime receiverSecondTarget = offset(receiverInitial, receiverEpsilon, 6);
         JavaTckSupport.check(compare(firstMessageTime, receiverFirstTarget) < 0
               && compare(receiverFirstTarget, secondMessageTime) < 0
               && compare(secondMessageTime, receiverSecondTarget) < 0,
            "multi-regulator TAR fixture times were not ordered 3 < 4 < 5 < 6");

         byte[] firstValue = new byte[] {0x41, 0x33};
         byte[] firstTag = new byte[] {0x54, 0x53, 0x4f, 0x2d, 0x41};
         ParameterHandleValueMap firstParameters =
            firstRegulator.getParameterHandleValueMapFactory().create(1);
         firstParameters.put(firstParameter, firstValue.clone());
         MessageRetractionReturn firstRetraction = firstRegulator.sendInteraction(
            firstInteraction, firstParameters, firstTag.clone(), firstMessageTime);

         byte[] secondValue = new byte[] {0x42, 0x35};
         byte[] secondTag = new byte[] {0x54, 0x53, 0x4f, 0x2d, 0x42};
         ParameterHandleValueMap secondParameters =
            secondRegulator.getParameterHandleValueMapFactory().create(1);
         secondParameters.put(secondParameter, secondValue.clone());
         MessageRetractionReturn secondRetraction = secondRegulator.sendInteraction(
            secondInteraction, secondParameters, secondTag.clone(), secondMessageTime);
         JavaTckSupport.check(firstRetraction != null
               && firstRetraction.retractionHandleIsValid && firstRetraction.handle != null
               && secondRetraction != null && secondRetraction.retractionHandleIsValid
               && secondRetraction.handle != null && receiverEvents.interactions.isEmpty(),
            "multi-regulator TAR did not retain both timestamped interactions");

         // Advance both regulators beyond their queued TSOs before receiver TAR.
         firstRegulator.timeAdvanceRequest(firstRegulatorTarget);
         secondRegulator.timeAdvanceRequest(secondRegulatorTarget);
         awaitAll(firstRegulator, secondRegulator, receiver, callbackModel,
            () -> firstEvents.grants.size() == 1 && secondEvents.grants.size() == 1,
            "both regulator TAR grants");
         JavaTckSupport.check(firstRegulatorTarget.equals(firstEvents.grants.get(0))
               && secondRegulatorTarget.equals(secondEvents.grants.get(0)),
            "a multi-regulator TAR sender advanced to the wrong logical time");

         receiver.timeAdvanceRequest(receiverFirstTarget);
         awaitAll(firstRegulator, secondRegulator, receiver, callbackModel,
            () -> receiverEvents.interactions.size() == 1
               && receiverEvents.grants.size() == 1,
            "TAR(4) delivery of queued TSO(3)");
         InteractionRecord firstDelivery = receiverEvents.interactions.get(0);
         JavaTckSupport.check(receiverEvents.interactions.size() == 1
               && receiverEvents.grants.size() == 1
               && receiverEvents.callbackOrder.equals(Arrays.asList("interaction", "grant")),
            "TAR(4) did not deliver only TSO(3) before its grant");
         checkDelivery(firstDelivery, receiverInteraction, receiverParameter,
            firstProducer, firstValue, firstTag, firstMessageTime,
            "TAR(4) changed TSO(3) data or producer");
         JavaTckSupport.check(receiverFirstTarget.equals(receiverEvents.grants.get(0)),
            "TAR(4) granted at the wrong logical time");

         // Returning from receiveInteraction acknowledges TSO(3); the next TAR
         // must still progress safely and deliver the remaining TSO(5) once.
         receiverEvents.callbackOrder.clear();
         JavaTckSupport.check(receiverEvents.interactions.size() == 1
               && receiverEvents.grants.size() == 1,
            "multi-regulator TAR delivered another TSO before the later request");
         receiver.timeAdvanceRequest(receiverSecondTarget);
         awaitAll(firstRegulator, secondRegulator, receiver, callbackModel,
            () -> receiverEvents.interactions.size() == 2
               && receiverEvents.grants.size() == 2,
            "TAR(6) progress after acknowledging TSO(3)");
         InteractionRecord secondDelivery = receiverEvents.interactions.get(1);
         JavaTckSupport.check(receiverEvents.interactions.size() == 2
               && receiverEvents.grants.size() == 2
               && receiverEvents.callbackOrder.equals(Arrays.asList("interaction", "grant")),
            "TAR(6) did not deliver the remaining TSO(5) once before its grant");
         checkDelivery(secondDelivery, receiverInteraction, receiverParameter,
            secondProducer, secondValue, secondTag, secondMessageTime,
            "TAR(6) changed TSO(5) data or producer");
         JavaTckSupport.check(receiverSecondTarget.equals(receiverEvents.grants.get(1)),
            "TAR(6) granted at the wrong logical time");

         receiver.disableTimeConstrained();
         receiverConstrained = false;
         firstRegulator.disableTimeRegulation();
         firstRegulating = false;
         secondRegulator.disableTimeRegulation();
         secondRegulating = false;
         receiver.unsubscribeInteractionClass(receiverInteraction);
         subscribed = false;
         firstRegulator.unpublishInteractionClass(firstInteraction);
         firstPublished = false;
         secondRegulator.unpublishInteractionClass(secondInteraction);
         secondPublished = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         secondRegulator.resignFederationExecution(ResignAction.NO_ACTION);
         secondJoined = false;
         firstRegulator.resignFederationExecution(ResignAction.NO_ACTION);
         firstJoined = false;
         firstRegulator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (firstRegulating && firstJoined) {
            try { firstRegulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (secondRegulating && secondJoined) {
            try { secondRegulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined && receiverInteraction != null) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (firstPublished && firstJoined && firstInteraction != null) {
            try { firstRegulator.unpublishInteractionClass(firstInteraction); }
            catch (Exception ignored) { }
         }
         if (secondPublished && secondJoined && secondInteraction != null) {
            try { secondRegulator.unpublishInteractionClass(secondInteraction); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) resign(receiver);
         if (secondJoined) resign(secondRegulator);
         if (firstJoined) resign(firstRegulator);
         if (created) {
            try { firstRegulator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (secondConnected) disconnect(secondRegulator);
         if (firstConnected) disconnect(firstRegulator);
      }
   }

   private static void checkDelivery(InteractionRecord delivery,
         InteractionClassHandle interaction, ParameterHandle parameter,
         FederateHandle producer, byte[] value, byte[] tag, LogicalTime<?, ?> time,
         String message) {
      JavaTckSupport.check(interaction.equals(delivery.interaction)
            && delivery.parameters.size() == 1
            && delivery.parameters.containsKey(parameter)
            && Arrays.equals(value, delivery.parameters.get(parameter))
            && Arrays.equals(tag, delivery.tag)
            && producer.equals(delivery.producer)
            && time.equals(delivery.time)
            && delivery.sentOrder == OrderType.TIMESTAMP
            && delivery.receivedOrder == OrderType.TIMESTAMP,
         message);
   }

   @FunctionalInterface
   private interface Condition { boolean isMet(); }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      awaitAll(model, () -> condition.getAsBoolean(), description, ambassador);
   }

   private static void awaitAll(RTIambassador first, RTIambassador second,
         RTIambassador receiver, CallbackModel model, Condition condition,
         String description) throws Exception {
      awaitAll(model, condition, description, first, second, receiver);
   }

   private static void awaitAll(CallbackModel model, Condition condition,
         String description, RTIambassador... ambassadors) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.isMet()) {
         if (model == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) ambassador.evokeCallback(0.0);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
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

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compare(LogicalTime<?, ?> left, LogicalTime<?, ?> right) {
      return ((LogicalTime) left).compareTo((LogicalTime) right);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK multi-regulator TAR callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class InteractionRecord {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;

      private InteractionRecord(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
      }
   }

   private static final class Recorder {
      private volatile int constrainedEnabled;
      private volatile int regulationEnabled;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<InteractionRecord> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) constrainedEnabled++;
               else if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  interactions.add(new InteractionRecord(arguments));
                  callbackOrder.add("interaction");
               }
               if (method.getDeclaringClass() == Object.class) {
                  return objectMethod(proxy, name, arguments);
               }
               return null;
            });
      }
   }
}
