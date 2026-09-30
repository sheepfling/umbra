package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
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

/** Delivers a queued timestamped directed interaction after its producer resigns. */
final class TimestampedDirectedInteractionSourceResignationTck {
   private TimestampedDirectedInteractionSourceResignationTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      Recorder clockRecorder = new Recorder();
      String federation = "java-tck-directed-source-resignation-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean clockConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean clockJoined = false;
      boolean ownerAttributesPublished = false;
      boolean publisherAttributesSubscribed = false;
      boolean receiverAttributesSubscribed = false;
      boolean directedPublished = false;
      boolean receiverDirectedSubscribed = false;
      boolean receiverConstrained = false;
      boolean publisherRegulating = false;
      boolean clockRegulating = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         clock.connect(clockRecorder.proxy(), callbackModel);
         clockConnected = true;

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-directed-target-owner", federation);
         ownerJoined = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-message-producer", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-directed-message-receiver", federation);
         receiverJoined = true;
         clock.joinFederationExecution("java-tck-directed-time-advancer", federation);
         clockJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle receiverAttribute =
            receiver.getAttributeHandle(receiverClass, attributeName);
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet receiverAttributes =
            JavaTckSupport.attributeSet(receiver, receiverAttribute);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerAttributesPublished = true;
         publisher.subscribeObjectClassAttributes(publisherClass, publisherAttributes);
         publisherAttributesSubscribed = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         receiverAttributesSubscribed = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         directedPublished = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         receiverDirectedSubscribed = true;

         ObjectInstanceHandle target = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(publisherRecorder.discoveries.contains(target)
               && receiverRecorder.discoveries.contains(target),
            "publisher and receiver did not both discover the owner-held directed target");

         LogicalTimeFactory<?, ?> ownerTimeFactory = owner.getTimeFactory();
         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> clockTimeFactory = clock.getTimeFactory();
         String factoryName = ownerTimeFactory.getName();
         JavaTckSupport.check(factoryName.equals(publisherTimeFactory.getName())
               && factoryName.equals(receiverTimeFactory.getName())
               && factoryName.equals(clockTimeFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTimeInterval publisherLookahead = offset(epsilon, 5);
         LogicalTimeInterval clockLookahead = offset(epsilon, 5);
         receiver.enableTimeConstrained();
         receiverConstrained = true;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabledCount == 1,
            "receiver did not report Time Constrained enable");
         publisher.enableTimeRegulation(publisherLookahead);
         publisherRegulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher did not report Time Regulation enable");
         clock.enableTimeRegulation(clockLookahead);
         clockRegulating = true;
         JavaTckSupport.drain(clock);
         JavaTckSupport.check(clockRecorder.timeRegulationEnabledCount == 1,
            "clock did not report Time Regulation enable");

         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(owner.queryLogicalTime())
               && initial.equals(receiver.queryLogicalTime())
               && initial.equals(clock.queryLogicalTime()),
            "federates began at different logical times");
         LogicalTime messageTime = offset(initial, epsilon, 6);
         LogicalTime receiverTarget = offset(initial, epsilon, 6);
         LogicalTime clockTarget = offset(initial, epsilon, 2);
         byte[] payload = new byte[] {0x44, 0x49, 0x52, 0x2D, 0x52};
         byte[] tag = new byte[] {0x44, 0x53, 0x4F, 0x2D, 0x52};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         MessageRetractionReturn sent = publisher.sendDirectedInteraction(
            publisherInteraction, target, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped directed send returned no valid retraction handle");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
            "receiver got the directed message before its time frontier");

         receiver.timeAdvanceRequest(receiverTarget);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timeAdvanceGrants.isEmpty(),
            "receiver was granted before the producer frontier");
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty()
               && receiverRecorder.timeAdvanceGrants.isEmpty(),
            "producer resignation released the queued directed callback too early");
         expectException(() -> publisher.retract(sent.handle), "FederateNotExecutionMember",
            "retracting through the resigned producer");

         clock.timeAdvanceRequest(clockTarget);
         JavaTckSupport.drain(clock);
         JavaTckSupport.check(clockRecorder.timeAdvanceGrants.size() == 1
               && clockTarget.equals(clockRecorder.timeAdvanceGrants.get(0)),
            "clock TAR did not grant its requested frontier");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1
               && receiverRecorder.timeAdvanceGrants.size() == 1,
            "clock frontier did not release exactly one directed delivery and receiver grant");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("receiveDirectedInteraction", "timeAdvanceGrant")),
            "directed delivery did not precede the receiver grant");
         JavaTckSupport.check(receiverTarget.equals(receiverRecorder.timeAdvanceGrants.get(0)),
            "receiver grant did not report the requested time");
         checkInteraction(receiverRecorder.interactions.get(0), receiverInteraction, target,
            receiverParameter, payload, tag, producer, messageTime, sent.handle, receiver);
         JavaTckSupport.check(receiverTarget.equals(receiver.queryLogicalTime())
               && clockTarget.equals(clock.queryLogicalTime()),
            "receiver or clock logical-time query disagreed with its grant");

         receiver.disableTimeConstrained();
         receiverConstrained = false;
         clock.disableTimeRegulation();
         clockRegulating = false;
      } finally {
         if (receiverConstrained) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (clockRegulating) {
            try { clock.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (receiverDirectedSubscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassDirectedInteractions(
               receiver.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         }
         if (receiverAttributesSubscribed && receiverJoined) {
            try {
               ObjectClassHandle objectClass = receiver.getObjectClassHandle(objectClassName);
               receiver.unsubscribeObjectClassAttributes(objectClass,
                  JavaTckSupport.attributeSet(receiver,
                     receiver.getAttributeHandle(objectClass, attributeName)));
            } catch (Exception ignored) { }
         }
         if (publisherAttributesSubscribed && publisherJoined) {
            try {
               ObjectClassHandle objectClass = publisher.getObjectClassHandle(objectClassName);
               publisher.unsubscribeObjectClassAttributes(objectClass,
                  JavaTckSupport.attributeSet(publisher,
                     publisher.getAttributeHandle(objectClass, attributeName)));
            } catch (Exception ignored) { }
         }
         if (directedPublished && publisherJoined) {
            try { publisher.unpublishObjectClassDirectedInteractions(
               publisher.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         }
         if (ownerAttributesPublished && ownerJoined) {
            try {
               ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
               owner.unpublishObjectClassAttributes(objectClass,
                  JavaTckSupport.attributeSet(owner,
                     owner.getAttributeHandle(objectClass, attributeName)));
            } catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (clockJoined) {
            try { clock.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try {
               owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            } catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (clockConnected) {
            try { clock.disconnect(); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTimeInterval offset(LogicalTimeInterval start, int steps)
         throws Exception {
      LogicalTimeInterval result = start;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(start);
      return result;
   }

   private static void checkInteraction(Interaction actual,
         InteractionClassHandle expectedClass, ObjectInstanceHandle expectedObject,
         ParameterHandle expectedParameter, byte[] expectedPayload, byte[] expectedTag,
         FederateHandle expectedProducer, LogicalTime<?, ?> expectedTime,
         MessageRetractionHandle expectedRetraction, RTIambassador inspector) throws Exception {
      JavaTckSupport.check(expectedClass.equals(actual.interactionClass)
            && expectedObject.equals(actual.object) && actual.parameters != null
            && actual.parameters.size() == 1
            && Arrays.equals(expectedPayload, actual.parameters.get(expectedParameter)),
         "directed callback returned the wrong target or parameter");
      JavaTckSupport.check(Arrays.equals(expectedTag, actual.tag)
            && expectedProducer.equals(actual.producer) && expectedTime.equals(actual.time),
         "directed callback did not preserve tag, producer, or timestamp");
      JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP,
         "directed callback returned the wrong sent or received order");
      JavaTckSupport.check(expectedRetraction.equals(actual.retraction)
            && actual.transportation != null
            && !inspector.getTransportationTypeName(actual.transportation).isEmpty(),
         "directed callback omitted the retraction or resolvable transport handle");
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
   private interface CheckedOperation { void run() throws Exception; }

   private static final class Interaction {
      private InteractionClassHandle interactionClass;
      private ObjectInstanceHandle object;
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
      private int timeConstrainedEnabledCount;
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabledCount;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
               } else if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  Interaction interaction = new Interaction();
                  interaction.interactionClass = (InteractionClassHandle) arguments[0];
                  interaction.object = (ObjectInstanceHandle) arguments[1];
                  interaction.parameters = ((ParameterHandleValueMap) arguments[2]).clone();
                  interaction.tag = ((byte[]) arguments[3]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[4];
                  interaction.producer = (FederateHandle) arguments[5];
                  interaction.time = (LogicalTime<?, ?>) arguments[6];
                  interaction.sentOrder = (OrderType) arguments[7];
                  interaction.receivedOrder = (OrderType) arguments[8];
                  interaction.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(interaction);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK source-resignation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
