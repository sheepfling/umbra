package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
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

/** Retracts a queued directed message before delivery and verifies subsequent delivery. */
final class TimestampedDirectedInteractionRetractionTck {
   private TimestampedDirectedInteractionRetractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRec = new Recorder();
      Recorder receiverRec = new Recorder();
      String federation = "java-tck-directed-retraction-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false, attributesPublished = false;
      boolean attributesSubscribed = false, directedPublished = false, directedSubscribed = false;
      boolean constrained = false, regulating = false;
      try {
         publisher.connect(publisherRec.proxy(), callbackModel); publisherConnected = true;
         receiver.connect(receiverRec.proxy(), callbackModel); receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-retraction-publisher", federation); publisherJoined = true;
         receiver.joinFederationExecution("java-tck-directed-retraction-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         InteractionClassHandle publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);
         publisher.publishObjectClassAttributes(publisherClass,
            JavaTckSupport.attributeSet(publisher, publisherAttribute)); attributesPublished = true;
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute)); attributesSubscribed = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction)); directedPublished = true;
         receiver.subscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction)); directedSubscribed = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRec.discoveries.contains(target),
            "receiver did not discover the publisher-held target");
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiver.queryLogicalTime()),
            "publisher and receiver began at different logical times");
         receiver.enableTimeConstrained(); constrained = true;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRec.timeConstrainedEnabled == 1,
            "receiver did not report Time Constrained enable");
         publisher.enableTimeRegulation(epsilon); regulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRec.timeRegulationEnabled == 1,
            "publisher did not report Time Regulation enable");

         LogicalTime firstTime = offset(initial, epsilon, 6);
         byte[] firstPayload = new byte[] {0x52, 0x45, 0x54};
         byte[] firstTag = new byte[] {0x52, 0x31};
         MessageRetractionReturn canceled = send(publisher, publisherInteraction,
            publisherParameter, target, firstPayload, firstTag, firstTime);
         JavaTckSupport.check(canceled != null && canceled.retractionHandleIsValid
               && canceled.handle != null, "first directed send returned no valid retraction handle");
         JavaTckSupport.check(receiverRec.interactions.isEmpty(),
            "timestamped directed interaction was delivered before the receiver advance");
         receiver.timeAdvanceRequest(firstTime);
         publisher.retract(canceled.handle);
         publisher.timeAdvanceRequest(firstTime);
         JavaTckSupport.drain(publisher); JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRec.grants.size() == 1
               && firstTime.equals(receiverRec.grants.get(0)),
            "receiver did not receive its first grant at the canceled message time");
         JavaTckSupport.check(receiverRec.interactions.isEmpty(),
            "retracted timestamped directed message was delivered");

         receiverRec.callbackOrder.clear();
         LogicalTime secondTime = offset(initial, epsilon, 8);
         byte[] secondPayload = new byte[] {0x44, 0x45, 0x4C};
         byte[] secondTag = new byte[] {0x52, 0x32};
         MessageRetractionReturn delivered = send(publisher, publisherInteraction,
            publisherParameter, target, secondPayload, secondTag, secondTime);
         JavaTckSupport.check(delivered != null && delivered.retractionHandleIsValid
               && delivered.handle != null, "second directed send returned no valid retraction handle");
         receiver.timeAdvanceRequest(secondTime);
         if (!publisherRec.grants.isEmpty()) publisher.timeAdvanceRequest(secondTime);
         JavaTckSupport.drain(publisher); JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRec.interactions.size() == 1 && receiverRec.grants.size() == 2,
            "later timestamped directed interaction did not deliver exactly once with its second grant");
         JavaTckSupport.check(firstTime.equals(receiverRec.grants.get(0))
               && secondTime.equals(receiverRec.grants.get(1)),
            "receiver grant sequence did not match the two requested times");
         checkDelivery(receiverRec, receiver, receiverInteraction, receiverParameter, target,
            secondPayload, secondTag, producer, secondTime, delivered.handle);
         expectException(() -> publisher.retract(delivered.handle), "MessageCanNoLongerBeRetracted");

         receiver.disableTimeConstrained(); constrained = false;
         publisher.disableTimeRegulation(); regulating = false;
      } finally {
         if (constrained) try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         if (regulating && publisherJoined) try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         if (directedSubscribed && receiverJoined) try { receiver.unsubscribeObjectClassDirectedInteractions(
            receiver.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         if (attributesSubscribed && receiverJoined) unsubscribe(receiver, objectClassName, attributeName);
         if (directedPublished && publisherJoined) try { publisher.unpublishObjectClassDirectedInteractions(
            publisher.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         if (attributesPublished && publisherJoined) try {
            ObjectClassHandle cls = publisher.getObjectClassHandle(objectClassName);
            publisher.unpublishObjectClassAttributes(cls,
               JavaTckSupport.attributeSet(publisher, publisher.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static MessageRetractionReturn send(RTIambassador publisher,
         InteractionClassHandle interaction, ParameterHandle parameter,
         ObjectInstanceHandle target, byte[] payload, byte[] tag, LogicalTime time) throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload);
      return publisher.sendDirectedInteraction(interaction, target, values, tag, time);
   }

   private static void unsubscribe(RTIambassador rti, String className, String attributeName) {
      try {
         ObjectClassHandle cls = rti.getObjectClassHandle(className);
         AttributeHandle attribute = rti.getAttributeHandle(cls, attributeName);
         rti.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(rti, attribute));
      } catch (Exception ignored) { }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static void checkDelivery(Recorder recorder, RTIambassador inspector,
         InteractionClassHandle interaction, ParameterHandle parameter, ObjectInstanceHandle target,
         byte[] payload, byte[] tag, FederateHandle producer, LogicalTime time,
         MessageRetractionHandle retraction) throws Exception {
      JavaTckSupport.check(recorder.callbackOrder.equals(Arrays.asList(
            "receiveDirectedInteraction", "timeAdvanceGrant")),
         "directed delivery did not precede the receiver's grant");
      Interaction actual = recorder.interactions.get(0);
      JavaTckSupport.check(interaction.equals(actual.interactionClass) && target.equals(actual.object)
            && actual.parameters != null && actual.parameters.size() == 1
            && Arrays.equals(payload, actual.parameters.get(parameter)),
         "directed callback returned the wrong target or parameter");
      JavaTckSupport.check(Arrays.equals(tag, actual.tag) && producer.equals(actual.producer)
            && time.equals(actual.time), "directed callback changed tag, producer, or timestamp");
      JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP,
         "directed callback returned the wrong order metadata");
      JavaTckSupport.check(retraction.equals(actual.retraction) && actual.transportation != null
            && !inspector.getTransportationTypeName(actual.transportation).isEmpty(),
         "directed callback omitted retraction or resolvable transport metadata");
   }

   private static void expectException(JavaTckSupport.CheckedOperation operation, String expected)
         throws Exception {
      try { operation.run(); }
      catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass())
            if (expected.equals(type.getSimpleName())) return;
         throw new AssertionError("retracting delivered message threw " + exception.getClass().getName()
            + " instead of " + expected, exception);
      }
      throw new AssertionError("retracting delivered message did not throw " + expected);
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
                  if ("toString".equals(name)) return "Java TCK directed retraction callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
