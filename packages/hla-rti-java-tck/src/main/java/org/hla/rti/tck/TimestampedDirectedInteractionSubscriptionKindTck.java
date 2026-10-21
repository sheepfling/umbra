package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
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
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks timestamped directed delivery for by-ownership and universal selectors. */
final class TimestampedDirectedInteractionSubscriptionKindTck {
   private TimestampedDirectedInteractionSubscriptionKindTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador ownershipSubscriber = factory.getRtiAmbassador();
      RTIambassador universalSubscriber = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder senderRecorder = new Recorder();
      Recorder ownershipRecorder = new Recorder();
      Recorder universalRecorder = new Recorder();
      RTIambassador[] ambassadors = {
         owner, sender, ownershipSubscriber, universalSubscriber
      };
      Recorder[] recorders = {
         ownerRecorder, senderRecorder, ownershipRecorder, universalRecorder
      };
      String federation = "java-tck-timestamped-directed-subscription-kind-"
         + UUID.randomUUID();
      boolean[] connected = new boolean[4];
      boolean[] joined = new boolean[4];
      boolean created = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         connected[0] = true;
         sender.connect(senderRecorder.proxy(), callbackModel);
         connected[1] = true;
         ownershipSubscriber.connect(ownershipRecorder.proxy(), callbackModel);
         connected[2] = true;
         universalSubscriber.connect(universalRecorder.proxy(), callbackModel);
         connected[3] = true;

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-timestamped-directed-owner", federation);
         joined[0] = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-timestamped-directed-sender", federation);
         joined[1] = true;
         ownershipSubscriber.joinFederationExecution(
            "java-tck-timestamped-directed-by-owner", federation);
         joined[2] = true;
         universalSubscriber.joinFederationExecution(
            "java-tck-timestamped-directed-universal", federation);
         joined[3] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle senderClass = sender.getObjectClassHandle(objectClassName);
         ObjectClassHandle ownershipClass =
            ownershipSubscriber.getObjectClassHandle(objectClassName);
         ObjectClassHandle universalClass =
            universalSubscriber.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle senderAttribute = sender.getAttributeHandle(senderClass, attributeName);
         AttributeHandle ownershipAttribute =
            ownershipSubscriber.getAttributeHandle(ownershipClass, attributeName);
         AttributeHandle universalAttribute =
            universalSubscriber.getAttributeHandle(universalClass, attributeName);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle senderInteraction =
            sender.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle ownershipInteraction =
            ownershipSubscriber.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle universalInteraction =
            universalSubscriber.getInteractionClassHandle(interactionClassName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle senderParameter = sender.getParameterHandle(senderInteraction, parameterName);
         ParameterHandle ownershipParameter =
            ownershipSubscriber.getParameterHandle(ownershipInteraction, parameterName);
         ParameterHandle universalParameter =
            universalSubscriber.getParameterHandle(universalInteraction, parameterName);
         JavaTckSupport.check(ownerAttribute != null && senderAttribute != null
               && ownershipAttribute != null && universalAttribute != null
               && ownerParameter != null && senderParameter != null
               && ownershipParameter != null && universalParameter != null,
            "timestamped directed-interaction handle lookup returned null");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet senderAttributes = JavaTckSupport.attributeSet(sender, senderAttribute);
         AttributeHandleSet ownershipAttributes =
            JavaTckSupport.attributeSet(ownershipSubscriber, ownershipAttribute);
         AttributeHandleSet universalAttributes =
            JavaTckSupport.attributeSet(universalSubscriber, universalAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         sender.subscribeObjectClassAttributes(senderClass, senderAttributes);
         ownershipSubscriber.subscribeObjectClassAttributes(ownershipClass, ownershipAttributes);
         universalSubscriber.subscribeObjectClassAttributes(universalClass, universalAttributes);

         InteractionClassHandleSet senderDirected =
            JavaTckSupport.interactionSet(sender, senderInteraction);
         InteractionClassHandleSet ownerDirected =
            JavaTckSupport.interactionSet(owner, ownerInteraction);
         InteractionClassHandleSet ownershipDirected =
            JavaTckSupport.interactionSet(ownershipSubscriber, ownershipInteraction);
         InteractionClassHandleSet universalDirected =
            JavaTckSupport.interactionSet(universalSubscriber, universalInteraction);
         sender.publishObjectClassDirectedInteractions(senderClass, senderDirected);
         owner.subscribeObjectClassDirectedInteractions(ownerClass, ownerDirected);
         ownershipSubscriber.subscribeObjectClassDirectedInteractions(
            ownershipClass, ownershipDirected);
         universalSubscriber.subscribeObjectClassDirectedInteractionsUniversally(
            universalClass, universalDirected);
         sender.changeInteractionOrderType(senderInteraction, OrderType.TIMESTAMP);

         ObjectInstanceHandle target = owner.registerObjectInstance(ownerClass);
         await(ambassadors, () -> senderRecorder.discoveries.contains(target)
               && ownershipRecorder.discoveries.contains(target)
               && universalRecorder.discoveries.contains(target),
            "timestamped directed target discovery");

         LogicalTimeFactory<?, ?> ownerTimeFactory = owner.getTimeFactory();
         LogicalTimeFactory<?, ?> senderTimeFactory = sender.getTimeFactory();
         LogicalTimeFactory<?, ?> ownershipTimeFactory = ownershipSubscriber.getTimeFactory();
         LogicalTimeFactory<?, ?> universalTimeFactory = universalSubscriber.getTimeFactory();
         JavaTckSupport.check(ownerTimeFactory.getName().equals(senderTimeFactory.getName())
               && ownerTimeFactory.getName().equals(ownershipTimeFactory.getName())
               && ownerTimeFactory.getName().equals(universalTimeFactory.getName()),
            "timestamped directed-interaction members selected different time factories");
         LogicalTimeInterval<?> lookahead =
            (LogicalTimeInterval<?>) senderTimeFactory.makeEpsilon();
         owner.enableTimeConstrained();
         ownershipSubscriber.enableTimeConstrained();
         universalSubscriber.enableTimeConstrained();
         sender.enableTimeRegulation(lookahead);
         await(ambassadors, () -> ownerRecorder.timeConstrainedEnabled
               && ownershipRecorder.timeConstrainedEnabled
               && universalRecorder.timeConstrainedEnabled
               && senderRecorder.timeRegulationEnabled,
            "timestamped directed-interaction time-role callbacks");

         LogicalTime<?, ?> ownerInitial = owner.queryLogicalTime();
         LogicalTime<?, ?> senderInitial = sender.queryLogicalTime();
         LogicalTime<?, ?> ownershipInitial = ownershipSubscriber.queryLogicalTime();
         LogicalTime<?, ?> universalInitial = universalSubscriber.queryLogicalTime();
         JavaTckSupport.check(ownerInitial.equals(senderInitial)
               && ownerInitial.equals(ownershipInitial)
               && ownerInitial.equals(universalInitial),
            "timestamped directed-interaction members began at different logical times");
         LogicalTime<?, ?> timestamp = offset(senderInitial, lookahead, 2);
         LogicalTime<?, ?> ownerTarget = offset(ownerInitial, lookahead, 2);
         LogicalTime<?, ?> ownershipTarget = offset(ownershipInitial, lookahead, 2);
         LogicalTime<?, ?> universalTarget = offset(universalInitial, lookahead, 2);
         JavaTckSupport.check(timestamp.equals(ownerTarget)
               && timestamp.equals(ownershipTarget) && timestamp.equals(universalTarget),
            "timestamped directed-interaction advance targets differ");

         byte[] payload = new byte[] {(byte) 0xD1, 0x52};
         byte[] tag = new byte[] {0x54, 0x53, 0x4F};
         ParameterHandleValueMap parameters =
            sender.getParameterHandleValueMapFactory().create(1);
         parameters.put(senderParameter, payload);
         MessageRetractionReturn sent = sender.sendDirectedInteraction(
            senderInteraction, target, parameters, tag, timestamp);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "timestamped directed send returned no valid retraction handle");
         JavaTckSupport.drain(owner);
         JavaTckSupport.drain(ownershipSubscriber);
         JavaTckSupport.drain(universalSubscriber);
         JavaTckSupport.check(ownerRecorder.interactions.isEmpty()
               && ownershipRecorder.interactions.isEmpty()
               && universalRecorder.interactions.isEmpty(),
            "timestamped directed interaction arrived before receiver advancement");
         for (Recorder recorder : recorders) recorder.callbackOrder.clear();

         owner.timeAdvanceRequest(ownerTarget);
         ownershipSubscriber.timeAdvanceRequest(ownershipTarget);
         universalSubscriber.timeAdvanceRequest(universalTarget);
         sender.timeAdvanceRequest(timestamp);
         await(ambassadors, () -> ownerRecorder.interactions.size() == 1
               && ownerRecorder.grants.size() == 1
               && ownershipRecorder.interactions.isEmpty()
               && ownershipRecorder.grants.size() == 1
               && universalRecorder.interactions.size() == 1
               && universalRecorder.grants.size() == 1
               && senderRecorder.grants.size() == 1,
            "timestamped directed-interaction delivery and grant callbacks");

         assertDelivery(ownerRecorder.interactions.get(0), ownerInteraction, target,
            ownerParameter, payload, tag, producer, timestamp, ownerRecorder.callbackOrder,
            "target-owner directed interaction");
         assertDelivery(universalRecorder.interactions.get(0), universalInteraction, target,
            universalParameter, payload, tag, producer, timestamp,
            universalRecorder.callbackOrder, "universal directed interaction");
         JavaTckSupport.check(ownerTarget.equals(ownerRecorder.grants.get(0))
               && ownershipTarget.equals(ownershipRecorder.grants.get(0))
               && universalTarget.equals(universalRecorder.grants.get(0))
               && timestamp.equals(senderRecorder.grants.get(0)),
            "timestamped directed-interaction grants returned the wrong logical times");

         ownershipSubscriber.publishObjectClassDirectedInteractions(
            ownershipClass, ownershipDirected);
         ownershipSubscriber.changeInteractionOrderType(
            ownershipInteraction, OrderType.TIMESTAMP);
         ownershipSubscriber.enableTimeRegulation(
            (LogicalTimeInterval<?>) ownershipTimeFactory.makeEpsilon());
         await(ambassadors, () -> ownershipRecorder.timeRegulationEnabled,
            "timestamped directed-interaction second sender's Time Regulation callback");

         LogicalTime<?, ?> senderLater = offset(senderInitial, lookahead, 4);
         LogicalTime<?, ?> ownershipLater = offset(ownershipInitial,
            (LogicalTimeInterval<?>) ownershipTimeFactory.makeEpsilon(), 4);
         JavaTckSupport.check(senderLater.equals(ownershipLater),
            "timestamped directed-interaction senders constructed different later times");
         ParameterHandleValueMap ownershipParameters =
            ownershipSubscriber.getParameterHandleValueMapFactory().create(1);
         ownershipParameters.put(ownershipParameter, payload);
         JavaTckSupport.expectFailure(
            () -> sender.sendDirectedInteraction(
               senderInteraction, target, parameters, tag, timestamp),
            "InvalidLogicalTime",
            "timestamped directed send below current logical time plus lookahead");
         LogicalTime<?, ?> senderAtLookaheadBoundary = offset(senderInitial, lookahead, 3);
         MessageRetractionReturn boundary = sender.sendDirectedInteraction(
            senderInteraction, target, parameters, tag, senderAtLookaheadBoundary);
         JavaTckSupport.check(boundary != null && boundary.retractionHandleIsValid
               && boundary.handle != null,
            "timestamped directed send at current logical time plus lookahead returned no handle");
         MessageRetractionReturn sameSender = sender.sendDirectedInteraction(
            senderInteraction, target, parameters, tag, senderLater);
         MessageRetractionReturn otherSender = ownershipSubscriber.sendDirectedInteraction(
            ownershipInteraction, target, ownershipParameters, tag, ownershipLater);
         JavaTckSupport.check(sameSender != null && sameSender.retractionHandleIsValid
               && sameSender.handle != null && otherSender != null
               && otherSender.retractionHandleIsValid && otherSender.handle != null
               && !sent.handle.equals(boundary.handle)
               && !sent.handle.equals(sameSender.handle)
               && !sent.handle.equals(otherSender.handle)
               && !boundary.handle.equals(sameSender.handle)
               && !boundary.handle.equals(otherSender.handle)
               && !sameSender.handle.equals(otherSender.handle),
            "timestamped directed sends did not return federation-unique retraction handles");

         universalSubscriber.publishObjectClassDirectedInteractions(
            universalClass, universalDirected);
         LogicalTime<?, ?> universalLater = offset(universalInitial,
            (LogicalTimeInterval<?>) universalTimeFactory.makeEpsilon(), 4);
         JavaTckSupport.check(senderLater.equals(universalLater),
            "timestamped directed-interaction non-regulating sender constructed a different time");
         ParameterHandleValueMap universalParameters =
            universalSubscriber.getParameterHandleValueMapFactory().create(1);
         universalParameters.put(universalParameter, payload);
         MessageRetractionReturn nonRegulating = universalSubscriber.sendDirectedInteraction(
            universalInteraction, target, universalParameters, tag, universalLater);
         JavaTckSupport.check(nonRegulating != null
               && !nonRegulating.retractionHandleIsValid,
            "timestamped directed send by a non-regulating federate returned a retraction handle");
      } finally {
         for (int i = 0; i < ambassadors.length; ++i) {
            if (joined[i]) {
               try {
                  ambassadors[i].resignFederationExecution(
                     ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
               } catch (Exception ignored) { }
            }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         for (int i = 0; i < ambassadors.length; ++i) {
            if (connected[i]) {
               try { ambassadors[i].disconnect(); } catch (Exception ignored) { }
            }
         }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval<?> interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = result.add(interval);
      return result;
   }

   private static void await(RTIambassador[] ambassadors,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         for (RTIambassador ambassador : ambassadors) JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), description + " did not complete");
   }

   private static void assertDelivery(Interaction actual,
         InteractionClassHandle expectedClass, ObjectInstanceHandle expectedTarget,
         ParameterHandle expectedParameter, byte[] expectedPayload, byte[] expectedTag,
         FederateHandle expectedProducer, LogicalTime<?, ?> expectedTime,
         List<String> callbackOrder, String description) throws Exception {
      JavaTckSupport.check(expectedClass.equals(actual.interactionClass)
            && expectedTarget.equals(actual.target) && actual.parameters.size() == 1
            && Arrays.equals(expectedPayload, actual.parameters.get(expectedParameter)),
         description + " carried the wrong interaction, target, or parameter");
      JavaTckSupport.check(Arrays.equals(expectedTag, actual.tag)
            && expectedProducer.equals(actual.producer) && expectedTime.equals(actual.time)
            && actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP && actual.retraction != null,
         description + " lost tag, producer, timestamp, order, or retraction metadata");
      JavaTckSupport.check(callbackOrder.equals(
            Arrays.asList("receiveDirectedInteraction", "timeAdvanceGrant")),
         description + " did not arrive before its Time Advance Grant");
   }

   private static final class Interaction {
      private InteractionClassHandle interactionClass;
      private ObjectInstanceHandle target;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile boolean timeConstrainedEnabled;
      private volatile boolean timeRegulationEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  Interaction interaction = new Interaction();
                  interaction.interactionClass = (InteractionClassHandle) arguments[0];
                  interaction.target = (ObjectInstanceHandle) arguments[1];
                  interaction.parameters = ((ParameterHandleValueMap) arguments[2]).clone();
                  interaction.tag = ((byte[]) arguments[3]).clone();
                  interaction.producer = (FederateHandle) arguments[5];
                  interaction.time = (LogicalTime<?, ?>) arguments[6];
                  interaction.sentOrder = (OrderType) arguments[7];
                  interaction.receivedOrder = (OrderType) arguments[8];
                  interaction.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(interaction);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped directed callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
