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

/** Delivers one queued timestamped directed interaction independently to two receivers. */
final class TimestampedDirectedInteractionSourceResignationFanoutTck {
   private TimestampedDirectedInteractionSourceResignationFanoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder ownerRec = new Recorder();
      Recorder publisherRec = new Recorder();
      Recorder firstRec = new Recorder();
      Recorder secondRec = new Recorder();
      Recorder clockRec = new Recorder();
      String federation = "java-tck-directed-source-resignation-fanout-" + UUID.randomUUID();
      boolean ownerConnected = false, publisherConnected = false;
      boolean firstConnected = false, secondConnected = false, clockConnected = false;
      boolean created = false, ownerJoined = false, publisherJoined = false;
      boolean firstJoined = false, secondJoined = false, clockJoined = false;
      boolean ownerPublished = false, publisherSubscribed = false;
      boolean firstSubscribed = false, secondSubscribed = false;
      boolean directedPublished = false, firstDirected = false, secondDirected = false;
      boolean firstConstrained = false, secondConstrained = false;
      boolean publisherRegulating = false, clockRegulating = false;
      try {
         owner.connect(ownerRec.proxy(), callbackModel); ownerConnected = true;
         publisher.connect(publisherRec.proxy(), callbackModel); publisherConnected = true;
         first.connect(firstRec.proxy(), callbackModel); firstConnected = true;
         second.connect(secondRec.proxy(), callbackModel); secondConnected = true;
         clock.connect(clockRec.proxy(), callbackModel); clockConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         owner.joinFederationExecution("java-tck-directed-fanout-owner", federation); ownerJoined = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-fanout-producer", federation); publisherJoined = true;
         first.joinFederationExecution("java-tck-directed-fanout-receiver-one", federation); firstJoined = true;
         second.joinFederationExecution("java-tck-directed-fanout-receiver-two", federation); secondJoined = true;
         clock.joinFederationExecution("java-tck-directed-fanout-clock", federation); clockJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = first.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = second.getObjectClassHandle(objectClassName);
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner,
            owner.getAttributeHandle(ownerClass, attributeName));
         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(publisher,
            publisher.getAttributeHandle(publisherClass, attributeName));
         AttributeHandleSet firstAttributes = JavaTckSupport.attributeSet(first,
            first.getAttributeHandle(firstClass, attributeName));
         AttributeHandleSet secondAttributes = JavaTckSupport.attributeSet(second,
            second.getAttributeHandle(secondClass, attributeName));
         InteractionClassHandle publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle firstInteraction = first.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle secondInteraction = second.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle firstParameter = first.getParameterHandle(firstInteraction, parameterName);
         ParameterHandle secondParameter = second.getParameterHandle(secondInteraction, parameterName);

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes); ownerPublished = true;
         publisher.subscribeObjectClassAttributes(publisherClass, publisherAttributes); publisherSubscribed = true;
         first.subscribeObjectClassAttributes(firstClass, firstAttributes); firstSubscribed = true;
         second.subscribeObjectClassAttributes(secondClass, secondAttributes); secondSubscribed = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction)); directedPublished = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         first.subscribeObjectClassDirectedInteractions(firstClass,
            JavaTckSupport.interactionSet(first, firstInteraction)); firstDirected = true;
         second.subscribeObjectClassDirectedInteractions(secondClass,
            JavaTckSupport.interactionSet(second, secondInteraction)); secondDirected = true;

         ObjectInstanceHandle target = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.drain(publisher); JavaTckSupport.drain(first); JavaTckSupport.drain(second);
         JavaTckSupport.check(publisherRec.discoveries.contains(target)
               && firstRec.discoveries.contains(target) && secondRec.discoveries.contains(target),
            "producer and both receivers did not discover the owner-held target");

         LogicalTimeFactory<?, ?> tf = publisher.getTimeFactory();
         String timeName = tf.getName();
         JavaTckSupport.check(timeName.equals(first.getTimeFactory().getName())
               && timeName.equals(second.getTimeFactory().getName())
               && timeName.equals(clock.getTimeFactory().getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) tf.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);
         first.enableTimeConstrained(); firstConstrained = true;
         second.enableTimeConstrained(); secondConstrained = true;
         JavaTckSupport.drain(first); JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRec.timeConstrainedEnabled == 1 && secondRec.timeConstrainedEnabled == 1,
            "both receivers must report Time Constrained enable");
         publisher.enableTimeRegulation(lookahead); publisherRegulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRec.timeRegulationEnabled == 1,
            "producer did not report Time Regulation enable");
         clock.enableTimeRegulation(lookahead); clockRegulating = true;
         JavaTckSupport.drain(clock);
         JavaTckSupport.check(clockRec.timeRegulationEnabled == 1,
            "clock did not report Time Regulation enable");

         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(first.queryLogicalTime())
               && initial.equals(second.queryLogicalTime()) && initial.equals(clock.queryLogicalTime()),
            "federates began at different logical times");
         LogicalTime messageTime = timeOffset(initial, epsilon, 6);
         LogicalTime firstTarget = timeOffset(initial, epsilon, 6);
         LogicalTime secondTarget = timeOffset(initial, epsilon, 6);
         LogicalTime firstClockTarget = timeOffset(initial, epsilon, 2);
         byte[] payload = new byte[] {0x44, 0x49, 0x52, 0x2D, 0x4D, 0x52};
         byte[] tag = new byte[] {0x44, 0x53, 0x4F, 0x2D, 0x4D, 0x52};
         ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         MessageRetractionReturn sent = publisher.sendDirectedInteraction(
            publisherInteraction, target, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped directed send returned no valid retraction handle");
         JavaTckSupport.drain(first); JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRec.interactions.isEmpty() && secondRec.interactions.isEmpty(),
            "a receiver got the directed message before its time frontier");

         first.timeAdvanceRequest(firstTarget);
         JavaTckSupport.drain(first);
         JavaTckSupport.check(firstRec.grants.isEmpty() && secondRec.grants.isEmpty(),
            "a receiver was granted before the producer frontier");
         publisher.resignFederationExecution(ResignAction.NO_ACTION); publisherJoined = false;
         JavaTckSupport.drain(first); JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRec.interactions.isEmpty() && secondRec.interactions.isEmpty()
               && firstRec.grants.isEmpty() && secondRec.grants.isEmpty(),
            "producer resignation prematurely released a queued receiver callback");
         expectNotExecutionMember(() -> publisher.retract(sent.handle));

         clock.timeAdvanceRequest(firstClockTarget);
         JavaTckSupport.drain(clock);
         JavaTckSupport.check(clockRec.grants.size() == 1
               && firstClockTarget.equals(clockRec.grants.get(0)), "clock missed its first requested grant");
         JavaTckSupport.drain(first); JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRec.interactions.size() == 1 && firstRec.grants.size() == 1,
            "first clock frontier did not release exactly one delivery and first receiver grant");
         JavaTckSupport.check(secondRec.interactions.isEmpty() && secondRec.grants.isEmpty(),
            "first receiver's frontier incorrectly released the second receiver");
         checkReceiver(firstRec, first, firstInteraction, firstParameter, target, payload, tag,
            producer, messageTime, sent.handle, firstTarget);

         LogicalTime secondClockTarget = timeOffset(initial, epsilon, 6);
         second.timeAdvanceRequest(secondTarget);
         clock.timeAdvanceRequest(secondClockTarget);
         JavaTckSupport.drain(clock); JavaTckSupport.drain(second);
         JavaTckSupport.check(secondRec.interactions.size() == 1 && secondRec.grants.size() == 1,
            "second clock frontier did not independently release the second receiver");
         checkReceiver(secondRec, second, secondInteraction, secondParameter, target, payload, tag,
            producer, messageTime, sent.handle, secondTarget);
         JavaTckSupport.check(firstRec.interactions.size() == 1 && firstRec.grants.size() == 1,
            "second receiver advancement caused duplicate delivery to the first receiver");
         JavaTckSupport.check(firstTarget.equals(first.queryLogicalTime())
               && secondTarget.equals(second.queryLogicalTime())
               && secondClockTarget.equals(clock.queryLogicalTime()),
            "receiver or clock logical-time query disagreed with its grant");

         first.disableTimeConstrained(); firstConstrained = false;
         second.disableTimeConstrained(); secondConstrained = false;
         clock.disableTimeRegulation(); clockRegulating = false;
      } finally {
         if (firstConstrained) try { first.disableTimeConstrained(); } catch (Exception ignored) { }
         if (secondConstrained) try { second.disableTimeConstrained(); } catch (Exception ignored) { }
         if (publisherRegulating && publisherJoined) try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         if (clockRegulating) try { clock.disableTimeRegulation(); } catch (Exception ignored) { }
         if (firstDirected && firstJoined) try { first.unsubscribeObjectClassDirectedInteractions(
            first.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         if (secondDirected && secondJoined) try { second.unsubscribeObjectClassDirectedInteractions(
            second.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         if (firstSubscribed && firstJoined) unsubscribe(first, objectClassName, attributeName);
         if (secondSubscribed && secondJoined) unsubscribe(second, objectClassName, attributeName);
         if (publisherSubscribed && publisherJoined) unsubscribe(publisher, objectClassName, attributeName);
         if (directedPublished && publisherJoined) try { publisher.unpublishObjectClassDirectedInteractions(
            publisher.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         if (ownerPublished && ownerJoined) try {
            ObjectClassHandle cls = owner.getObjectClassHandle(objectClassName);
            owner.unpublishObjectClassAttributes(cls,
               JavaTckSupport.attributeSet(owner, owner.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (firstJoined) try { first.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         if (secondJoined) try { second.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         if (clockJoined) try { clock.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         if (ownerJoined) try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         if (clockConnected) try { clock.disconnect(); } catch (Exception ignored) { }
         if (secondConnected) try { second.disconnect(); } catch (Exception ignored) { }
         if (firstConnected) try { first.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void unsubscribe(RTIambassador rti, String objectClassName, String attributeName) {
      try {
         ObjectClassHandle cls = rti.getObjectClassHandle(objectClassName);
         AttributeHandle attr = rti.getAttributeHandle(cls, attributeName);
         rti.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(rti, attr));
      } catch (Exception ignored) { }
   }

   private static LogicalTime timeOffset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTimeInterval result = interval;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(interval);
      return result;
   }

   private static void checkReceiver(Recorder recorder, RTIambassador inspector,
         InteractionClassHandle interactionClass, ParameterHandle parameter,
         ObjectInstanceHandle target, byte[] payload, byte[] tag, FederateHandle producer,
         LogicalTime expectedTime, MessageRetractionHandle retraction, LogicalTime expectedGrant)
         throws Exception {
      JavaTckSupport.check(recorder.callbackOrder.equals(
            Arrays.asList("receiveDirectedInteraction", "timeAdvanceGrant")),
         "directed delivery did not precede its receiver's grant");
      JavaTckSupport.check(expectedGrant.equals(recorder.grants.get(0)),
         "receiver grant did not report its requested time");
      Interaction actual = recorder.interactions.get(0);
      JavaTckSupport.check(interactionClass.equals(actual.interactionClass)
            && target.equals(actual.object) && actual.parameters != null
            && actual.parameters.size() == 1 && Arrays.equals(payload, actual.parameters.get(parameter)),
         "directed callback returned the wrong target or parameter");
      JavaTckSupport.check(Arrays.equals(tag, actual.tag) && producer.equals(actual.producer)
            && expectedTime.equals(actual.time), "directed callback lost tag, producer, or timestamp");
      JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP,
         "directed callback returned the wrong sent or received order");
      JavaTckSupport.check(retraction.equals(actual.retraction) && actual.transportation != null
            && !inspector.getTransportationTypeName(actual.transportation).isEmpty(),
         "directed callback omitted retraction or resolvable transportation metadata");
   }

   private static void expectNotExecutionMember(CheckedOperation operation) throws Exception {
      try { operation.run(); }
      catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass())
            if ("FederateNotExecutionMember".equals(type.getSimpleName())) return;
         throw new AssertionError("retraction after resignation threw "
            + exception.getClass().getName() + " instead of FederateNotExecutionMember", exception);
      }
      throw new AssertionError("retraction after resignation did not throw FederateNotExecutionMember");
   }

   @FunctionalInterface private interface CheckedOperation { void run() throws Exception; }

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
      private int timeRegulationEnabled;
      private int timeConstrainedEnabled;
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new ArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) ++timeRegulationEnabled;
               else if ("timeConstrainedEnabled".equals(name)) ++timeConstrainedEnabled;
               else if ("discoverObjectInstance".equals(name) && arguments != null && arguments.length >= 3)
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               else if ("receiveDirectedInteraction".equals(name) && arguments != null && arguments.length == 10) {
                  Interaction event = new Interaction();
                  event.interactionClass = (InteractionClassHandle) arguments[0];
                  event.object = (ObjectInstanceHandle) arguments[1];
                  event.parameters = ((ParameterHandleValueMap) arguments[2]).clone();
                  event.tag = ((byte[]) arguments[3]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[4];
                  event.producer = (FederateHandle) arguments[5];
                  event.time = (LogicalTime<?, ?>) arguments[6];
                  event.sentOrder = (OrderType) arguments[7];
                  event.receivedOrder = (OrderType) arguments[8];
                  event.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(event); callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]); callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed fanout callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
