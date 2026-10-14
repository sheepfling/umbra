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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** P2 callback-control checks for the Request Retraction callback after TSO delivery. */
final class CallbackControlsTimestampedRetractionTck {
   private CallbackControlsTimestampedRetractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String objectClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-callback-tso-retraction-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean regulating = false;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-retraction-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-retraction-receiver", federation);
         receiverJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            "publisher and receiver resolved different interaction or parameter handles");
         publisher.publishInteractionClass(publisherInteraction);
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "publisher and receiver selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon);
         regulating = true;
         await(callbackModel, () -> publisherRecorder.timeRegulationEnabled != null,
            publisher, objectClassName);

         LogicalTime target = add(publisher.queryLogicalTime(), intervalOffset(epsilon, 3));
         byte[] payload = new byte[] {0x52, 0x45, 0x54};
         byte[] tag = new byte[] {0x52, 0x45, 0x54};
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload);
         MessageRetractionReturn retraction = publisher.sendInteraction(
            publisherInteraction, values, tag, target);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null && retraction.handle.encodedLength() > 0,
            "timestamped Send Interaction returned an invalid retraction handle");

         await(callbackModel, () -> receiverRecorder.interactions.size() == 1,
            receiver, objectClassName);
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1,
            "timestamped interaction was delivered more than once before retraction");
         Interaction interaction = receiverRecorder.interactions.get(0);
         JavaTckSupport.check(receiverInteraction.equals(interaction.interactionClass)
               && interaction.parameters.size() == 1
               && Arrays.equals(payload, interaction.parameters.get(receiverParameter))
               && Arrays.equals(tag, interaction.tag)
               && producer.equals(interaction.producer)
               && target.equals(interaction.time)
               && interaction.sentOrder == OrderType.TIMESTAMP
               && interaction.receivedOrder == OrderType.RECEIVE
               && interaction.transportation != null
               && !receiver.getTransportationTypeName(interaction.transportation).isEmpty()
               && retraction.handle.equals(interaction.retraction),
            "timestamped interaction delivery returned incorrect standard callback metadata");

         receiver.disableCallbacks();
         publisher.retract(retraction.handle);
         service(receiver, callbackModel, objectClassName);
         JavaTckSupport.check(receiverRecorder.retractions.isEmpty(),
            "disabled callbacks exposed requestRetraction");

         receiver.enableCallbacks();
         await(callbackModel, () -> receiverRecorder.retractions.size() == 1,
            receiver, objectClassName);
         JavaTckSupport.check(receiverRecorder.retractions.size() == 1
               && retraction.handle.equals(receiverRecorder.retractions.get(0))
               && receiverRecorder.retractions.get(0).encodedLength() > 0,
            "re-enabled requestRetraction returned the wrong or invalid handle");
         JavaTckSupport.check(Arrays.asList("receiveInteraction", "requestRetraction")
               .equals(receiverRecorder.callbackOrder),
            "requestRetraction callback sequence did not follow delivered interaction");
      } finally {
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (receiverJoined && receiverInteraction != null) {
            try { receiver.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static LogicalTime add(LogicalTime start, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) start.add(interval);
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private static void await(CallbackModel model, Condition condition,
         RTIambassador ambassador, String objectClassName) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         service(ambassador, model, objectClassName);
         if (condition.isMet()) return;
         Thread.yield();
      }
      throw new AssertionError("timestamped interaction callback did not arrive before deadline");
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Interaction {
      private InteractionClassHandle interactionClass;
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
      private LogicalTime<?, ?> timeRegulationEnabled;
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<MessageRetractionHandle> retractions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Interaction interaction = new Interaction();
                  interaction.interactionClass = (InteractionClassHandle) arguments[0];
                  interaction.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  interaction.tag = ((byte[]) arguments[2]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[3];
                  interaction.producer = (FederateHandle) arguments[4];
                  interaction.time = (LogicalTime<?, ?>) arguments[6];
                  interaction.sentOrder = (OrderType) arguments[7];
                  interaction.receivedOrder = (OrderType) arguments[8];
                  interaction.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(interaction);
                  callbackOrder.add(name);
               } else if ("requestRetraction".equals(name)
                     && arguments != null && arguments.length == 1) {
                  retractions.add((MessageRetractionHandle) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK TSO retraction recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
