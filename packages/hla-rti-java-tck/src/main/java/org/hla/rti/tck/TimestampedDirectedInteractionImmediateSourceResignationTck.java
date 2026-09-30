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

/** Confirms immediate delivery of an accepted timestamped directed message after its producer resigns. */
final class TimestampedDirectedInteractionImmediateSourceResignationTck {
   private TimestampedDirectedInteractionImmediateSourceResignationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerRec = new Recorder();
      Recorder senderRec = new Recorder();
      Recorder receiverRec = new Recorder();
      String federation = "java-tck-directed-immediate-resign-" + UUID.randomUUID();
      boolean ownerConnected = false, senderConnected = false, receiverConnected = false;
      boolean created = false, ownerJoined = false, senderJoined = false, receiverJoined = false;
      boolean ownerPublished = false, senderSubscribed = false, receiverSubscribed = false;
      boolean directedPublished = false, receiverDirected = false;
      try {
         owner.connect(ownerRec.proxy(), callbackModel); ownerConnected = true;
         sender.connect(senderRec.proxy(), callbackModel); senderConnected = true;
         receiver.connect(receiverRec.proxy(), callbackModel); receiverConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         owner.joinFederationExecution("java-tck-directed-immediate-owner", federation); ownerJoined = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-directed-immediate-producer", federation); senderJoined = true;
         receiver.joinFederationExecution("java-tck-directed-immediate-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle senderClass = sender.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle senderAttribute = sender.getAttributeHandle(senderClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         InteractionClassHandle senderInteraction = sender.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle senderParameter = sender.getParameterHandle(senderInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);

         owner.publishObjectClassAttributes(ownerClass, JavaTckSupport.attributeSet(owner, ownerAttribute));
         ownerPublished = true;
         sender.subscribeObjectClassAttributes(senderClass,
            JavaTckSupport.attributeSet(sender, senderAttribute)); senderSubscribed = true;
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute)); receiverSubscribed = true;
         sender.publishObjectClassDirectedInteractions(senderClass,
            JavaTckSupport.interactionSet(sender, senderInteraction)); directedPublished = true;
         sender.changeInteractionOrderType(senderInteraction, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction)); receiverDirected = true;

         ObjectInstanceHandle target = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.drain(sender); JavaTckSupport.drain(receiver);
         JavaTckSupport.check(senderRec.discoveries.contains(target)
               && receiverRec.discoveries.contains(target),
            "sender and receiver did not discover the owner-held target");

         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         sender.enableTimeRegulation(epsilon);
         JavaTckSupport.drain(sender);
         JavaTckSupport.check(senderRec.timeRegulationEnabled == 1,
            "sender did not report Time Regulation enable");
         LogicalTime initial = sender.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiver.queryLogicalTime()),
            "sender and receiver began at different logical times");
         LogicalTime messageTime = offset(initial, epsilon, 6);
         byte[] payload = new byte[] {0x49, 0x4D, 0x4D, 0x2D, 0x52};
         byte[] tag = new byte[] {0x49, 0x4D, 0x4D, 0x2D, 0x52, 0x45, 0x53};
         ParameterHandleValueMap parameters = sender.getParameterHandleValueMapFactory().create(1);
         parameters.put(senderParameter, payload);
         MessageRetractionReturn sent = sender.sendDirectedInteraction(
            senderInteraction, target, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped directed send returned no valid retraction handle");

         sender.resignFederationExecution(ResignAction.NO_ACTION); senderJoined = false;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRec.interactions.size() == 1,
            "accepted timestamped directed interaction was not delivered exactly once after sender resignation");
         JavaTckSupport.check(receiverRec.retractions == 0,
            "sender resignation produced an unexpected message retraction callback");
         Interaction actual = receiverRec.interactions.get(0);
         JavaTckSupport.check(receiverRec.callbackOrder.equals(
               Arrays.asList("receiveDirectedInteraction")),
            "receiver callback route did not contain exactly one directed delivery");
         JavaTckSupport.check(receiverInteraction.equals(actual.interactionClass)
               && target.equals(actual.object) && actual.parameters != null
               && actual.parameters.size() == 1
               && Arrays.equals(payload, actual.parameters.get(receiverParameter)),
            "directed callback returned the wrong interaction, target, or parameter");
         JavaTckSupport.check(Arrays.equals(tag, actual.tag) && producer.equals(actual.producer)
               && messageTime.equals(actual.time),
            "directed callback lost tag, producer, or timestamp");
         JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
               && actual.receivedOrder == OrderType.RECEIVE,
            "directed callback returned the wrong sent or received order");
         JavaTckSupport.check(sent.handle.equals(actual.retraction)
               && actual.transportation != null
               && !receiver.getTransportationTypeName(actual.transportation).isEmpty(),
            "directed callback omitted the retraction or resolvable transport handle");
      } finally {
         if (receiverDirected && receiverJoined) try { receiver.unsubscribeObjectClassDirectedInteractions(
            receiver.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         if (receiverSubscribed && receiverJoined) unsubscribe(receiver, objectClassName, attributeName);
         if (senderSubscribed && senderJoined) unsubscribe(sender, objectClassName, attributeName);
         if (directedPublished && senderJoined) try { sender.unpublishObjectClassDirectedInteractions(
            sender.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         if (ownerPublished && ownerJoined) try {
            ObjectClassHandle cls = owner.getObjectClassHandle(objectClassName);
            owner.unpublishObjectClassAttributes(cls,
               JavaTckSupport.attributeSet(owner, owner.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (senderJoined) try { sender.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (senderConnected) try { sender.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void unsubscribe(RTIambassador rti, String objectClassName, String attributeName) {
      try {
         ObjectClassHandle cls = rti.getObjectClassHandle(objectClassName);
         rti.unsubscribeObjectClassAttributes(cls,
            JavaTckSupport.attributeSet(rti, rti.getAttributeHandle(cls, attributeName)));
      } catch (Exception ignored) { }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

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
      private int retractions;
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) ++timeRegulationEnabled;
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
               } else if (name.startsWith("requestRetraction") || "requestRetraction".equals(name)) {
                  ++retractions;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK immediate source-resignation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
