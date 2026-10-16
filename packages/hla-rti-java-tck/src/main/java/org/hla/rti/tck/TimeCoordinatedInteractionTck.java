package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Verifies that a timestamped interaction is coordinated with receiver time. */
final class TimeCoordinatedInteractionTck {
   private TimeCoordinatedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-time-coordinated-interaction-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-time-coordinated-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-time-coordinated-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherClass =
            publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle receiverClass =
            receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherClass, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverClass, parameterName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherParameter != null && receiverParameter != null,
            "time-coordinated interaction lookup returned an invalid standard handle");
         publisher.publishInteractionClass(publisherClass);
         published = true;
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "time-coordinated interaction members selected different logical-time factories");
         LogicalTimeInterval lookahead =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime publisherAtTwo = offset(publisherInitial, lookahead, 2);
         LogicalTime receiverAtTwo = offset(
            receiverInitial, (LogicalTimeInterval) receiverFactory.makeEpsilon(), 2);
         LogicalTime publisherAtFive = offset(publisherInitial, lookahead, 5);
         LogicalTime receiverAtFive = offset(
            receiverInitial, (LogicalTimeInterval) receiverFactory.makeEpsilon(), 5);

         receiver.enableTimeConstrained();
         publisher.enableTimeRegulation(lookahead);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabled
               && publisherRecorder.timeRegulationEnabled,
            "time-coordinated interaction time-role callbacks were not observed");

         byte[] payload = new byte[] {0x54, 0x49, 0x4D, 0x45};
         byte[] tag = new byte[] {0x43, 0x4F, 0x4F, 0x52};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         MessageRetractionReturn accepted =
            publisher.sendInteraction(publisherClass, parameters, tag, publisherAtFive);
         JavaTckSupport.check(accepted != null && accepted.retractionHandleIsValid
               && accepted.handle != null,
            "time-coordinated timestamped interaction returned no valid retraction handle");

         publisher.timeAdvanceRequest(publisherAtTwo);
         receiver.timeAdvanceRequest(receiverAtTwo);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(publisherRecorder.timeAdvanceGrantTimes.size() == 1
               && publisherRecorder.timeAdvanceGrantTimes.get(0).equals(publisherAtTwo),
            "time-coordinated publisher received the wrong pre-message grant");
         JavaTckSupport.check(receiverRecorder.timeAdvanceGrantTimes.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.get(0).equals(receiverAtTwo),
            "time-coordinated receiver received the wrong pre-message grant");
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
            "timestamped interaction was delivered before its logical timestamp");

         receiverRecorder.callbackOrder.clear();
         publisher.timeAdvanceRequest(publisherAtFive);
         receiver.timeAdvanceRequest(receiverAtFive);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(publisherRecorder.timeAdvanceGrantTimes.size() == 2
               && publisherRecorder.timeAdvanceGrantTimes.get(1).equals(publisherAtFive),
            "time-coordinated publisher received the wrong message-time grant");
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.size() == 2
               && receiverRecorder.timeAdvanceGrantTimes.get(1).equals(receiverAtFive),
            "time-coordinated interaction delivery or receiver grant was missing");

         ObservedInteraction delivered = receiverRecorder.interactions.get(0);
         byte[] observedValue = delivered.parameters.get(receiverParameter);
         JavaTckSupport.check(delivered.interactionClass.equals(receiverClass)
               && delivered.producer.equals(producer)
               && delivered.parameters.size() == 1
               && Arrays.equals(observedValue, payload)
               && Arrays.equals(delivered.tag, tag)
               && delivered.time.equals(receiverAtFive)
               && delivered.transportation != null
               && delivered.sentOrder == OrderType.TIMESTAMP
               && delivered.receivedOrder == OrderType.TIMESTAMP,
            "time-coordinated callback did not preserve standard delivery data");
         int interactionIndex = receiverRecorder.callbackOrder.indexOf("receiveInteraction");
         int grantIndex = receiverRecorder.callbackOrder.indexOf("timeAdvanceGrant");
         JavaTckSupport.check(interactionIndex >= 0 && grantIndex > interactionIndex,
            "time-coordinated interaction did not precede the matching grant callback");

         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1,
            "time-coordinated interaction was delivered more than once");
      } finally {
         if (subscribed && receiverJoined) {
            try {
               receiver.unsubscribeInteractionClass(
                  receiver.getInteractionClassHandle(interactionName));
            } catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try {
               publisher.unpublishInteractionClass(
                  publisher.getInteractionClassHandle(interactionName));
            } catch (Exception ignored) { }
         }
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

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static final class ObservedInteraction {
      private final InteractionClassHandle interactionClass;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;

      private ObservedInteraction(InteractionClassHandle interactionClass,
            ParameterHandleValueMap parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            LogicalTime<?, ?> time, OrderType sentOrder, OrderType receivedOrder) {
         this.interactionClass = interactionClass;
         this.parameters = parameters;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
      }
   }

   private static final class Recorder {
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<ObservedInteraction> interactions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  interactions.add(new ObservedInteraction(
                     (InteractionClassHandle) arguments[0],
                     ((ParameterHandleValueMap) arguments[1]).clone(),
                     ((byte[]) arguments[2]).clone(),
                     (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4],
                     (LogicalTime<?, ?>) arguments[6],
                     (OrderType) arguments[7],
                     (OrderType) arguments[8]));
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK time-coordinated interaction callbacks";
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
