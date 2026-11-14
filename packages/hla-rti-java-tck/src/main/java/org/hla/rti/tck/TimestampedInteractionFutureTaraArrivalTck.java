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
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies a future timestamped interaction arriving during a pending TARA. */
final class TimestampedInteractionFutureTaraArrivalTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;

   private TimestampedInteractionFutureTaraArrivalTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      Recorder publisherRecorder = new Recorder();
      String federation = "java-tck-future-tara-interaction-" + UUID.randomUUID();
      boolean receiverConnected = false;
      boolean publisherConnected = false;
      boolean created = false;
      boolean receiverJoined = false;
      boolean publisherJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean receiverConstrained = false;
      boolean publisherRegulating = false;
      InteractionClassHandle receiverInteraction = null;
      InteractionClassHandle publisherInteraction = null;
      try {
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         receiver.joinFederationExecution("java-tck-future-tara-receiver", federation);
         receiverJoined = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-future-tara-publisher", federation);
         publisherJoined = true;

         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         JavaTckSupport.check(receiverInteraction.equals(publisherInteraction)
               && receiverParameter.equals(publisherParameter),
            "future TARA interaction handles differed between federates");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(receiverTimeFactory.getName().equals(publisherTimeFactory.getName()),
            "future TARA participants selected different logical-time factories");
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverTimeFactory.makeEpsilon();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         JavaTckSupport.check(receiverInitial.equals(publisherInitial),
            "future TARA participants began at different logical times");

         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(callbackModel, "receiver Time Constrained callback",
            () -> receiverRecorder.timeConstrainedEnabled, receiver);
         publisher.enableTimeRegulation(publisherEpsilon);
         publisherRegulating = true;
         await(callbackModel, "publisher Time Regulation callback",
            () -> publisherRecorder.timeRegulationEnabled, publisher);

         LogicalTime publisherCurrent = offset(publisherInitial, publisherEpsilon, 2);
         publisher.timeAdvanceRequest(publisherCurrent);
         await(callbackModel, "publisher initial time advance grant",
            () -> publisherRecorder.grants.size() == 1, publisher);
         JavaTckSupport.check(publisherCurrent.equals(publisherRecorder.grants.get(0)),
            "publisher initial grant had the wrong logical time");

         LogicalTime messageTime = offset(publisherCurrent, publisherEpsilon, 3);
         LogicalTime requestBoundary = offset(receiverInitial, receiverEpsilon, 6);
         LogicalTime publisherAdvanceTarget = offset(publisherCurrent, publisherEpsilon, 4);
         receiver.timeAdvanceRequestAvailable(requestBoundary);
         requirePending(callbackModel, receiver, receiverRecorder,
            "TARA completed before the future interaction arrived");

         byte[] payload = new byte[] {0x54, 0x41, 0x52, 0x41, 0x35};
         byte[] tag = new byte[] {0x54, 0x41, 0x52, 0x41};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         MessageRetractionReturn sent = publisher.sendInteraction(
            publisherInteraction, parameters, tag.clone(), messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "future TARA interaction returned no valid retraction handle");
         requirePending(callbackModel, receiver, receiverRecorder,
            "TARA delivered the future interaction before its timestamp became safe");

         publisher.timeAdvanceRequest(publisherAdvanceTarget);
         await(callbackModel, "interaction delivery and both time advance grants",
            () -> publisherRecorder.grants.size() == 2
               && receiverRecorder.interactions.size() == 1
               && receiverRecorder.grants.size() == 1,
            publisher, receiver);

         JavaTckSupport.check(publisherRecorder.grants.size() == 2
               && receiverRecorder.interactions.size() == 1
               && receiverRecorder.grants.size() == 1,
            "future TARA interaction returned duplicate or missing callbacks");
         Interaction event = receiverRecorder.interactions.get(0);
         JavaTckSupport.check(receiverInteraction.equals(event.interaction)
               && event.parameters.size() == 1
               && Arrays.equals(payload, event.parameters.get(receiverParameter))
               && Arrays.equals(tag, event.tag)
               && producer.equals(event.producer)
               && messageTime.equals(event.time)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == OrderType.TIMESTAMP
               && sent.handle.equals(event.retraction)
               && event.transportation != null
               && !receiver.getTransportationTypeName(event.transportation).isEmpty(),
            "future TARA interaction callback changed its identity or metadata");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("interaction", "grant"))
               && requestBoundary.equals(receiverRecorder.grants.get(0)),
            "TARA granted before the interaction or at the wrong requested time");
         JavaTckSupport.check(publisherAdvanceTarget.equals(publisherRecorder.grants.get(1)),
            "publisher follow-up grant had the wrong logical time");
         JavaTckSupport.check(requestBoundary.equals(receiver.queryLogicalTime()),
            "Query Logical Time did not return the TARA grant time");

         receiver.disableTimeConstrained();
         receiverConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
         receiver.unsubscribeInteractionClass(receiverInteraction);
         subscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         published = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         receiver.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined && receiverInteraction != null) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static void await(CallbackModel model, String description, Condition condition,
         RTIambassador... ambassadors) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.isMet()) {
         if (model == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) JavaTckSupport.drain(ambassador);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   private static void requirePending(CallbackModel model, RTIambassador receiver,
         Recorder recorder, String description) throws Exception {
      long deadline = System.nanoTime() + 40_000_000L;
      while (System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(receiver);
         JavaTckSupport.check(recorder.interactions.isEmpty() && recorder.grants.isEmpty(),
            description);
         Thread.sleep(1L);
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK future TARA interaction callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  event.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.time = (LogicalTime<?, ?>) arguments[6];
                  event.sentOrder = (OrderType) arguments[7];
                  event.receivedOrder = (OrderType) arguments[8];
                  event.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(event);
                  callbackOrder.add("interaction");
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  return objectMethod(proxy, name, arguments);
               }
               return null;
            });
      }
   }
}
