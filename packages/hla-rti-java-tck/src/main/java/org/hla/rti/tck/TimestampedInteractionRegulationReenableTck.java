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

/** Preserves a queued interaction while the producer's regulation is re-enabled. */
final class TimestampedInteractionRegulationReenableTck {
   private TimestampedInteractionRegulationReenableTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-interaction-regulation-reenable-"
         + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean created = false;
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
            "java-tck-regulation-reenable-producer", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-regulation-reenable-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publishedClass =
            publisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle publishedParameter =
            publisher.getParameterHandle(publishedClass, parameterName);
         publisher.publishInteractionClass(publishedClass);
         published = true;
         publisher.changeInteractionOrderType(publishedClass, OrderType.TIMESTAMP);
         InteractionClassHandle receiverClass =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverClass, parameterName);
         receiver.subscribeInteractionClass(receiverClass);
         subscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         String factoryName = timeFactory.getName();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(factoryName.equals(receiverTimeFactory.getName()),
            "publisher and receiver selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabledCount == 1,
            "receiver did not report its initial Time Constrained enable callback");
         publisher.enableTimeRegulation(epsilon);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher did not report its initial Time Regulation enable callback");

         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiverInitial),
            "publisher and receiver began at different logical times");
         LogicalTime messageTime = offset(initial, epsilon, 5);
         LogicalTime publisherTarget = offset(initial, epsilon, 2);
         LogicalTimeInterval changedLookahead = offset(epsilon, 3);

         byte[] payload = new byte[] {(byte) 0xd1, 0x5a};
         byte[] tag = new byte[] {0x43, 0x48, 0x47, 0x4c};
         MessageRetractionReturn sent = send(
            publisher, publishedClass, publishedParameter, payload, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped send returned no valid retraction handle");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
            "queued interaction was delivered before its constrained grant");

         publisher.disableTimeRegulation();
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationDisabledCount == 1,
            "publisher did not report Time Regulation disable");
         publisher.enableTimeRegulation(changedLookahead);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabledCount == 2,
            "publisher did not report its second Time Regulation enable callback");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
            "queued interaction was delivered during Time Regulation re-enable");
         JavaTckSupport.check(changedLookahead.equals(publisher.queryLookahead()),
            "Query Lookahead did not return the changed lookahead");

         receiverRecorder.callbackOrder.clear();
         publisherRecorder.callbackOrder.clear();
         receiver.timeAdvanceRequest(messageTime);
         publisher.timeAdvanceRequest(publisherTarget);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1,
            "re-enabled publisher's queued interaction was not delivered exactly once");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "queued interaction was not delivered before the receiver's grant");
         JavaTckSupport.check(messageTime.equals(receiverRecorder.timeAdvanceGrant)
               && publisherTarget.equals(publisherRecorder.timeAdvanceGrant),
            "time-advance grants did not report their requested logical times");
         checkInteraction(receiverRecorder.interactions.get(0), receiverClass,
            receiverParameter, payload, tag, producer, messageTime, sent.handle,
            receiver, "re-enabled timestamped delivery");
         JavaTckSupport.check(messageTime.equals(receiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "logical-time queries did not match the granted times");
         expectException(() -> publisher.retract(sent.handle), "MessageCanNoLongerBeRetracted",
            "retracting the interaction after its delivery grant");

         receiver.disableTimeConstrained();
         publisher.disableTimeRegulation();
         JavaTckSupport.drain(publisher);
      } finally {
         if (published) {
            try {
               publisher.unpublishInteractionClass(
                  publisher.getInteractionClassHandle(interactionClassName));
            } catch (Exception ignored) { }
         }
         if (subscribed) {
            try {
               receiver.unsubscribeInteractionClass(
                  receiver.getInteractionClassHandle(interactionClassName));
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

   private static LogicalTimeInterval offset(LogicalTimeInterval start, int steps)
         throws Exception {
      LogicalTimeInterval result = start;
      for (int i = 1; i < steps; ++i) {
         result = (LogicalTimeInterval) result.add(start);
      }
      return result;
   }

   private static void checkInteraction(Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         byte[] expectedPayload, byte[] expectedTag, FederateHandle expectedProducer,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(expectedClass.equals(interaction.interactionClass)
            && interaction.parameters != null && interaction.parameters.size() == 1
            && Arrays.equals(expectedPayload, interaction.parameters.get(expectedParameter)),
         description + " returned the wrong class or parameter payload");
      JavaTckSupport.check(Arrays.equals(expectedTag, interaction.tag),
         description + " did not preserve the user tag");
      JavaTckSupport.check(expectedProducer.equals(interaction.producer)
            && expectedTime.equals(interaction.time),
         description + " did not preserve producer or timestamp");
      JavaTckSupport.check(interaction.sentOrder == OrderType.TIMESTAMP
            && interaction.receivedOrder == OrderType.TIMESTAMP,
         description + " returned the wrong sent or received order");
      JavaTckSupport.check(expectedRetraction.equals(interaction.retraction)
            && interaction.transportation != null
            && !inspector.getTransportationTypeName(interaction.transportation).isEmpty(),
         description + " omitted the retraction or resolvable transport handle");
   }

   private static void expectException(CheckedOperation operation, String expected,
         String description) throws Exception {
      try {
         operation.run();
      } catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass()) {
            if (expected.equals(type.getSimpleName())) return;
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
      private int timeRegulationEnabledCount;
      private int timeRegulationDisabledCount;
      private int timeConstrainedEnabledCount;
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabledCount;
               } else if ("timeRegulationDisabled".equals(name)) {
                  ++timeRegulationDisabledCount;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
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
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regulation re-enable callbacks";
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
