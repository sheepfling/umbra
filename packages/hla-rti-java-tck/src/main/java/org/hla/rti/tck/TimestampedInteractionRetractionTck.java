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

/** Portable timestamped interaction delivery and retraction callback checks. */
final class TimestampedInteractionRetractionTck {
   private TimestampedInteractionRetractionTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador immediate = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder immediateRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      String federation = "java-tck-timestamped-interaction-retraction-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean immediateConnected = false;
      boolean constrainedConnected = false;
      boolean publisherJoined = false;
      boolean immediateJoined = false;
      boolean constrainedJoined = false;
      boolean created = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         immediate.connect(immediateRecorder.proxy(), callbackModel);
         immediateConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-retraction-producer", federation);
         publisherJoined = true;
         immediate.joinFederationExecution("java-tck-retraction-immediate", federation);
         immediateJoined = true;
         constrained.joinFederationExecution("java-tck-retraction-constrained", federation);
         constrainedJoined = true;

         InteractionClassHandle publishedClass =
            publisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle publishedParameter =
            publisher.getParameterHandle(publishedClass, parameterName);
         publisher.publishInteractionClass(publishedClass);
         publisher.changeInteractionOrderType(publishedClass, OrderType.TIMESTAMP);

         InteractionClassHandle immediateClass =
            immediate.getInteractionClassHandle(interactionClassName);
         ParameterHandle immediateParameter =
            immediate.getParameterHandle(immediateClass, parameterName);
         immediate.subscribeInteractionClass(immediateClass);
         InteractionClassHandle constrainedClass =
            constrained.getInteractionClassHandle(interactionClassName);
         ParameterHandle constrainedParameter =
            constrained.getParameterHandle(constrainedClass, parameterName);
         constrained.subscribeInteractionClass(constrainedClass);

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         String timeFactoryName = timeFactory.getName();
         LogicalTimeFactory<?, ?> immediateTimeFactory = immediate.getTimeFactory();
         LogicalTimeFactory<?, ?> constrainedTimeFactory = constrained.getTimeFactory();
         JavaTckSupport.check(timeFactoryName.equals(immediateTimeFactory.getName())
               && timeFactoryName.equals(constrainedTimeFactory.getName()),
            "timestamped interaction members selected different logical-time factories");
         LogicalTimeInterval lookahead = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(lookahead);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabled,
            "timeRegulationEnabled did not cross the producer callback interface");
         constrained.enableTimeConstrained();
         JavaTckSupport.drain(constrained);
         JavaTckSupport.check(constrainedRecorder.timeConstrainedEnabled,
            "timeConstrainedEnabled did not cross the receiver callback interface");

         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime firstTime = offset(initial, lookahead, 3);
         LogicalTime firstReceiverTime = offset(constrained.queryLogicalTime(), lookahead, 3);
         JavaTckSupport.check(firstTime.equals(firstReceiverTime),
            "publisher and constrained receiver began at different logical times");
         byte[] firstPayload = new byte[] {0x01, 0x02, 0x03};
         byte[] firstTag = new byte[] {0x11, 0x12};
         MessageRetractionReturn first = send(
            publisher, publishedClass, publishedParameter, firstPayload, firstTag, firstTime);
         JavaTckSupport.check(first != null && first.retractionHandleIsValid && first.handle != null,
            "first timestamped Send Interaction returned no valid retraction handle");
         JavaTckSupport.drain(immediate);
         JavaTckSupport.drain(constrained);
         checkInteraction(immediateRecorder, 0, immediateClass, immediateParameter,
            firstPayload, firstTag, producer, firstTime, first.handle, OrderType.RECEIVE,
            "immediate recipient");
         JavaTckSupport.check(constrainedRecorder.interactions.isEmpty(),
            "constrained recipient received the timestamped interaction before its grant");

         publisher.retract(first.handle);
         JavaTckSupport.drain(immediate);
         JavaTckSupport.drain(constrained);
         JavaTckSupport.check(immediateRecorder.retractions.size() == 1
               && first.handle.equals(immediateRecorder.retractions.get(0)),
            "delivered interaction did not produce one matching Request Retraction callback");
         JavaTckSupport.check(constrainedRecorder.retractions.isEmpty()
               && constrainedRecorder.interactions.isEmpty(),
            "pending constrained copy produced a callback when the interaction was retracted");

         constrained.timeAdvanceRequest(firstReceiverTime);
         publisher.timeAdvanceRequest(firstTime);
         JavaTckSupport.drain(constrained);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(constrainedRecorder.interactions.isEmpty()
               && constrainedRecorder.retractions.isEmpty(),
            "retracted queued interaction was delivered at the first constrained grant");
         JavaTckSupport.check(firstTime.equals(publisherRecorder.timeAdvanceGrant)
               && firstReceiverTime.equals(constrainedRecorder.timeAdvanceGrant),
            "first grants did not report their requested logical times");

         LogicalTime secondTime = offset(firstTime, lookahead, 3);
         LogicalTime secondReceiverTime = offset(firstReceiverTime, lookahead, 3);
         byte[] secondPayload = new byte[] {0x21, 0x22, 0x23};
         byte[] secondTag = new byte[] {0x31, 0x32};
         MessageRetractionReturn second = send(
            publisher, publishedClass, publishedParameter, secondPayload, secondTag, secondTime);
         JavaTckSupport.check(second != null && second.retractionHandleIsValid && second.handle != null,
            "second timestamped Send Interaction returned no valid retraction handle");
         JavaTckSupport.drain(immediate);
         checkInteraction(immediateRecorder, 1, immediateClass, immediateParameter,
            secondPayload, secondTag, producer, secondTime, second.handle, OrderType.RECEIVE,
            "second immediate delivery");
         JavaTckSupport.check(immediateRecorder.retractions.size() == 1,
            "unretracted second interaction produced a Request Retraction callback");

         constrainedRecorder.callbackOrder.clear();
         constrained.timeAdvanceRequest(secondReceiverTime);
         publisher.timeAdvanceRequest(secondTime);
         JavaTckSupport.drain(constrained);
         JavaTckSupport.drain(publisher);
         checkInteraction(constrainedRecorder, 0, constrainedClass, constrainedParameter,
            secondPayload, secondTag, producer, secondTime, second.handle, OrderType.TIMESTAMP,
            "constrained delivery");
         JavaTckSupport.check(constrainedRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "timestamped interaction was not delivered before its matching grant");
         JavaTckSupport.check(secondTime.equals(constrainedRecorder.timeAdvanceGrant)
               && secondTime.equals(constrained.queryLogicalTime()),
            "constrained grant or logical-time query returned the wrong time");
         expectException(() -> publisher.retract(second.handle), "MessageCanNoLongerBeRetracted",
            "retracting the already delivered timestamped interaction");
      } finally {
         if (constrainedJoined) {
            try { constrained.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (immediateJoined) {
            try { immediate.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (constrainedConnected) {
            try { constrained.disconnect(); } catch (Exception ignored) { }
         }
         if (immediateConnected) {
            try { immediate.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static MessageRetractionReturn send(RTIambassador ambassador,
         InteractionClassHandle interaction, ParameterHandle parameter, byte[] payload,
         byte[] tag, LogicalTime<?, ?> time) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload);
      return ambassador.sendInteraction(interaction, values, tag, time);
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void checkInteraction(Recorder recorder, int index,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         byte[] expectedPayload, byte[] expectedTag, FederateHandle expectedProducer,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         OrderType expectedOrder, String description) {
      JavaTckSupport.check(recorder.interactions.size() == index + 1,
         description + " received the wrong number of interactions");
      Interaction interaction = recorder.interactions.get(index);
      JavaTckSupport.check(expectedClass.equals(interaction.interactionClass)
            && interaction.parameters != null && interaction.parameters.size() == 1
            && Arrays.equals(expectedPayload, interaction.parameters.get(expectedParameter)),
         description + " received the wrong class or parameter payload");
      JavaTckSupport.check(Arrays.equals(expectedTag, interaction.tag),
         description + " did not preserve the user tag");
      JavaTckSupport.check(expectedProducer.equals(interaction.producer)
            && expectedTime.equals(interaction.time),
         description + " did not preserve producer or logical timestamp");
      JavaTckSupport.check(interaction.sentOrder == OrderType.TIMESTAMP
            && interaction.receivedOrder == expectedOrder,
         description + " reported the wrong sent or received order");
      JavaTckSupport.check(expectedRetraction.equals(interaction.retraction)
            && interaction.transportation != null,
         description + " did not preserve its retraction or transportation handle");
   }

   private static void expectException(CheckedOperation operation, String expected,
         String description) throws Exception {
      try {
         operation.run();
      } catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass()) {
            if (expected.equals(type.getSimpleName())) {
               return;
            }
         }
         throw new AssertionError(description + " threw " + exception.getClass().getName()
            + " instead of " + expected, exception);
      }
      throw new AssertionError(description + " did not throw " + expected);
   }

   @FunctionalInterface
   private interface CheckedOperation {
      void run() throws Exception;
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
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<MessageRetractionHandle> retractions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private LogicalTime<?, ?> timeAdvanceGrant;

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
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK interaction retraction callbacks";
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
