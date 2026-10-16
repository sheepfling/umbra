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
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** Portable Flush Queue future-input and post-grant timestamp-bound behavior. */
final class TimestampedInteractionFlushQueueFutureInputTck {
   private TimestampedInteractionFlushQueueFutureInputTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-interaction-flush-queue-future-input-"
         + java.util.UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean created = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-future-input-producer", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-future-input-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherClass =
            publisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherClass, parameterName);
         InteractionClassHandle receiverClass =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverClass, parameterName);
         publisher.publishInteractionClass(publisherClass);
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);
         receiver.publishInteractionClass(receiverClass);
         receiver.changeInteractionOrderType(receiverClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "timestamped interaction members selected different logical-time factories");
         receiver.enableTimeConstrained();
         JavaTckSupport.drain(receiver);
         publisher.enableTimeRegulation(epsilon);
         JavaTckSupport.drain(publisher);
         receiver.enableTimeRegulation((LogicalTimeInterval) receiver.getTimeFactory().makeEpsilon());
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabled
               && publisherRecorder.timeRegulationEnabled
               && receiverRecorder.timeRegulationEnabled,
            "time-management enable callbacks were not delivered through the API");

         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime firstTime = offset(initial, epsilon, 1);
         LogicalTime secondTime = offset(initial, epsilon, 12);
         LogicalTime publisherTarget = offset(initial, epsilon, 4);
         LogicalTime requestBoundary = offset(receiverInitial, epsilon, 10);
         byte[] firstPayload = new byte[] {0x46, 0x55, 0x54, 0x55, 0x52, 0x45};
         byte[] secondPayload = new byte[] {0x46, 0x55, 0x54, 0x55, 0x52, 0x45, 0x2d, 0x32};
         byte[] firstTag = new byte[] {0x46, 0x51, 0x31};
         byte[] secondTag = new byte[] {0x46, 0x51, 0x32};
         MessageRetractionReturn firstRetraction = publisher.sendInteraction(
            publisherClass, oneParameter(publisher, publisherParameter, firstPayload),
            firstTag, firstTime);
         MessageRetractionReturn secondRetraction = publisher.sendInteraction(
            publisherClass, oneParameter(publisher, publisherParameter, secondPayload),
            secondTag, secondTime);
         checkRetraction(firstRetraction, "first future interaction");
         checkRetraction(secondRetraction, "second future interaction");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
            "future interactions were delivered before Flush Queue Request");

         receiver.flushQueueRequest(requestBoundary);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
               "future interactions were delivered before callback servicing");
         }
         publisher.timeAdvanceRequest(publisherTarget);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.drain(publisher);

         JavaTckSupport.check(publisherRecorder.timeAdvanceGrantTimes.size() == 1
               && publisherTarget.equals(publisherRecorder.timeAdvanceGrantTimes.get(0)),
            "publisher received the wrong time-advance grant");
         JavaTckSupport.check(receiverRecorder.interactions.size() == 2
               && receiverRecorder.flushQueueGrantTimes.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.isEmpty(),
            "Flush Queue Request returned the wrong interaction or grant callback counts");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(Arrays.asList(
               "receiveInteraction", "receiveInteraction", "flushQueueGrant")),
            "Flush Queue Request granted before both queued future interactions");
         checkInteraction(receiver, receiverRecorder.interactions.get(0), receiverClass,
            receiverParameter, firstPayload, firstTag, producer, firstTime,
            firstRetraction.handle, "first future interaction");
         checkInteraction(receiver, receiverRecorder.interactions.get(1), receiverClass,
            receiverParameter, secondPayload, secondTag, producer, secondTime,
            secondRetraction.handle, "second future interaction");

         LogicalTime flushGrant = receiverRecorder.flushQueueGrantTime;
         LogicalTime optimistic = receiverRecorder.flushQueueOptimisticTime;
         JavaTckSupport.check(flushGrant != null && optimistic != null
               && firstTime.equals(optimistic)
               && compareTime(flushGrant, receiverInitial) >= 0
               && compareTime(flushGrant, requestBoundary) <= 0
               && compareTime(optimistic, flushGrant) >= 0
               && flushGrant.equals(receiver.queryLogicalTime()),
            "Flush Queue Grant/query returned an inconsistent actual or optimistic frontier");

         LogicalTimeInterval receiverLookahead = receiver.queryLookahead();
         JavaTckSupport.check(receiverLookahead.equals(receiver.getTimeFactory().makeEpsilon()),
            "receiver lookahead changed unexpectedly");
         LogicalTime postGrantSendTime = offset(flushGrant, receiverLookahead, 1);
         MessageRetractionReturn postGrantSend = receiver.sendInteraction(receiverClass,
            oneParameter(receiver, receiverParameter, firstPayload), firstTag,
            postGrantSendTime);
         JavaTckSupport.check(postGrantSend != null && postGrantSend.retractionHandleIsValid
               && postGrantSend.handle != null,
            "post-grant TSO send at current logical time plus lookahead returned no retraction handle");
         JavaTckSupport.check(flushGrant.equals(receiver.queryLogicalTime()),
            "post-grant TSO send changed the federate's logical time");
      } finally {
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static ParameterHandleValueMap oneParameter(RTIambassador ambassador,
         ParameterHandle parameter, byte[] payload) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload);
      return values;
   }

   private static void checkRetraction(MessageRetractionReturn result, String description) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null, description + " returned no valid retraction handle");
   }

   private static void checkInteraction(RTIambassador receiver, Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         byte[] expectedPayload, byte[] expectedTag, FederateHandle expectedProducer,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         String description) throws Exception {
      JavaTckSupport.check(expectedClass.equals(interaction.interactionClass)
            && interaction.parameters != null && interaction.parameters.size() == 1
            && Arrays.equals(expectedPayload, interaction.parameters.get(expectedParameter)),
         description + " returned the wrong class or parameter payload");
      JavaTckSupport.check(Arrays.equals(expectedTag, interaction.tag),
         description + " did not preserve its user tag");
      JavaTckSupport.check(expectedProducer.equals(interaction.producer)
            && expectedTime.equals(interaction.time),
         description + " returned the wrong producer or logical timestamp");
      JavaTckSupport.check(interaction.sentOrder == OrderType.TIMESTAMP
            && interaction.receivedOrder == OrderType.TIMESTAMP,
         description + " returned the wrong order metadata");
      JavaTckSupport.check(interaction.regions == null || interaction.regions.isEmpty(),
         description + " unexpectedly included region metadata");
      JavaTckSupport.check(expectedRetraction.equals(interaction.retraction)
            && interaction.transportation != null
            && !receiver.getTransportationTypeName(interaction.transportation).isEmpty(),
         description + " did not preserve retraction or transportation metadata");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compareTime(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   private static final class Interaction {
      private InteractionClassHandle interactionClass;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private RegionHandleSet regions;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private LogicalTime<?, ?> flushQueueGrantTime;
      private LogicalTime<?, ?> flushQueueOptimisticTime;

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
                  Interaction interaction = new Interaction();
                  interaction.interactionClass = (InteractionClassHandle) arguments[0];
                  interaction.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  interaction.tag = ((byte[]) arguments[2]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[3];
                  interaction.producer = (FederateHandle) arguments[4];
                  interaction.regions = (RegionHandleSet) arguments[5];
                  interaction.time = (LogicalTime<?, ?>) arguments[6];
                  interaction.sentOrder = (OrderType) arguments[7];
                  interaction.receivedOrder = (OrderType) arguments[8];
                  interaction.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(interaction);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               } else if ("flushQueueGrant".equals(name)
                     && arguments != null && arguments.length == 2) {
                  flushQueueGrantTime = (LogicalTime<?, ?>) arguments[0];
                  flushQueueOptimisticTime = (LogicalTime<?, ?>) arguments[1];
                  flushQueueGrantTimes.add(flushQueueGrantTime);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK future-input callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
