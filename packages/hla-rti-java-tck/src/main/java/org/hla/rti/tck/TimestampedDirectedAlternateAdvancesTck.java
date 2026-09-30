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

/** Exercises timestamped directed delivery through FQR, TARA, and NMRA. */
final class TimestampedDirectedAlternateAdvancesTck {
   private TimestampedDirectedAlternateAdvancesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador flushReceiver = factory.getRtiAmbassador();
      RTIambassador taraReceiver = factory.getRtiAmbassador();
      RTIambassador nmraReceiver = factory.getRtiAmbassador();
      Recorder publisherRec = new Recorder();
      Recorder flushRec = new Recorder();
      Recorder taraRec = new Recorder();
      Recorder nmraRec = new Recorder();
      String federation = "java-tck-timestamped-directed-alternates-" + UUID.randomUUID();
      boolean publisherConnected = false, flushConnected = false;
      boolean taraConnected = false, nmraConnected = false;
      boolean created = false, publisherJoined = false, flushJoined = false;
      boolean taraJoined = false, nmraJoined = false;
      boolean attributesPublished = false, directedPublished = false;
      boolean flushAttributesSubscribed = false, taraAttributesSubscribed = false;
      boolean nmraAttributesSubscribed = false, flushDirectedSubscribed = false;
      boolean taraDirectedSubscribed = false, nmraDirectedSubscribed = false;
      boolean publisherRegulating = false, flushConstrained = false;
      boolean taraConstrained = false, nmraConstrained = false;
      try {
         publisher.connect(publisherRec.proxy(), callbackModel); publisherConnected = true;
         flushReceiver.connect(flushRec.proxy(), callbackModel); flushConnected = true;
         taraReceiver.connect(taraRec.proxy(), callbackModel); taraConnected = true;
         nmraReceiver.connect(nmraRec.proxy(), callbackModel); nmraConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-advance-publisher", federation); publisherJoined = true;
         flushReceiver.joinFederationExecution("java-tck-directed-fqr", federation);
         flushJoined = true;
         taraReceiver.joinFederationExecution("java-tck-directed-tara", federation);
         taraJoined = true;
         nmraReceiver.joinFederationExecution("java-tck-directed-nmra", federation);
         nmraJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         publisher.publishObjectClassAttributes(publisherClass,
            JavaTckSupport.attributeSet(publisher, publisherAttribute));
         attributesPublished = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         directedPublished = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         RTIambassador[] receivers = {flushReceiver, taraReceiver, nmraReceiver};
         Recorder[] receiverRecs = {flushRec, taraRec, nmraRec};
         ParameterHandle[] receiverParameters = new ParameterHandle[receivers.length];
         InteractionClassHandle[] receiverInteractions = new InteractionClassHandle[receivers.length];
         AttributeHandle[] receiverAttributes = new AttributeHandle[receivers.length];
         for (int i = 0; i < receivers.length; ++i) {
            ObjectClassHandle cls = receivers[i].getObjectClassHandle(objectClassName);
            receiverAttributes[i] = receivers[i].getAttributeHandle(cls, attributeName);
            receiverInteractions[i] = receivers[i].getInteractionClassHandle(interactionClassName);
            receiverParameters[i] = receivers[i].getParameterHandle(
               receiverInteractions[i], parameterName);
            receivers[i].subscribeObjectClassAttributes(cls,
               JavaTckSupport.attributeSet(receivers[i], receiverAttributes[i]));
            if (i == 0) flushAttributesSubscribed = true;
            else if (i == 1) taraAttributesSubscribed = true;
            else nmraAttributesSubscribed = true;
            receivers[i].subscribeObjectClassDirectedInteractions(cls,
               JavaTckSupport.interactionSet(receivers[i], receiverInteractions[i]));
            if (i == 0) flushDirectedSubscribed = true;
            else if (i == 1) taraDirectedSubscribed = true;
            else nmraDirectedSubscribed = true;
         }

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         for (RTIambassador receiver : receivers) JavaTckSupport.drain(receiver);
         for (int i = 0; i < receivers.length; ++i) {
            checkDiscovery(receiverRecs[i], target, publisherClass, producer,
               "directed alternate-advance receiver " + i);
         }

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         String factoryName = timeFactory.getName();
         for (RTIambassador receiver : receivers) {
            JavaTckSupport.check(factoryName.equals(receiver.getTimeFactory().getName()),
               "directed alternate-advance members selected different logical-time factories");
         }
         publisher.enableTimeRegulation(epsilon); publisherRegulating = true;
         flushReceiver.enableTimeConstrained(); flushConstrained = true;
         taraReceiver.enableTimeConstrained(); taraConstrained = true;
         nmraReceiver.enableTimeConstrained(); nmraConstrained = true;
         JavaTckSupport.drain(publisher);
         for (RTIambassador receiver : receivers) JavaTckSupport.drain(receiver);
         JavaTckSupport.check(publisherRec.timeRegulationEnabled
               && flushRec.timeConstrainedEnabled && taraRec.timeConstrainedEnabled
               && nmraRec.timeConstrainedEnabled,
            "a time regulation/constrained role callback was not observed");

         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime messageTime = offset(initial, epsilon, 3);
         LogicalTime publisherTarget = offset(initial, epsilon, 2);
         LogicalTime requestBoundary = offset(initial, epsilon, 5);
         byte[] payload = new byte[] {0x61, 0x62, 0x63};
         byte[] tag = new byte[] {0x41, 0x44, 0x56};
         ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory()
            .create(1);
         parameters.put(publisherParameter, payload);
         MessageRetractionReturn sent = publisher.sendDirectedInteraction(
            publisherInteraction, target, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "timestamped directed send returned no valid retraction handle");
         for (RTIambassador receiver : receivers) JavaTckSupport.drain(receiver);
         JavaTckSupport.check(flushRec.interactions.isEmpty() && taraRec.interactions.isEmpty()
               && nmraRec.interactions.isEmpty(),
            "timestamped directed interaction arrived before an advance request");
         flushRec.callbackOrder.clear(); taraRec.callbackOrder.clear();
         nmraRec.callbackOrder.clear();

         flushReceiver.flushQueueRequest(requestBoundary);
         taraReceiver.timeAdvanceRequestAvailable(messageTime);
         nmraReceiver.nextMessageRequestAvailable(requestBoundary);
         publisher.timeAdvanceRequest(publisherTarget);
         awaitCompletion(publisher, receivers, publisherRec, receiverRecs);

         checkDelivery(flushRec, flushReceiver, receiverInteractions[0], receiverParameters[0],
            target, payload, tag, producer, messageTime, sent.handle, "Flush Queue Request");
         checkDelivery(taraRec, taraReceiver, receiverInteractions[1], receiverParameters[1],
            target, payload, tag, producer, messageTime, sent.handle,
            "Time Advance Request Available");
         checkDelivery(nmraRec, nmraReceiver, receiverInteractions[2], receiverParameters[2],
            target, payload, tag, producer, messageTime, sent.handle,
            "Next Message Request Available");
         JavaTckSupport.check(publisherRec.timeAdvanceGrantTimes.size() == 1
               && publisherTarget.equals(publisherRec.timeAdvanceGrantTimes.get(0)),
            "publisher TAR did not grant the requested logical time");
         JavaTckSupport.check(flushRec.flushQueueGrantTimes.size() == 1
               && flushRec.timeAdvanceGrantTimes.isEmpty()
               && flushRec.callbackOrder.equals(Arrays.asList(
                  "receiveDirectedInteraction", "flushQueueGrant")),
            "Flush Queue Request did not deliver before its sole flush grant");
         JavaTckSupport.check(taraRec.flushQueueGrantTimes.isEmpty()
               && taraRec.timeAdvanceGrantTimes.size() == 1
               && taraRec.callbackOrder.equals(Arrays.asList(
                  "receiveDirectedInteraction", "timeAdvanceGrant")),
            "TARA did not deliver before its sole time-advance grant");
         JavaTckSupport.check(nmraRec.flushQueueGrantTimes.isEmpty()
               && nmraRec.timeAdvanceGrantTimes.size() == 1
               && nmraRec.callbackOrder.equals(Arrays.asList(
                  "receiveDirectedInteraction", "timeAdvanceGrant")),
            "NMRA did not deliver before its sole time-advance grant");
         JavaTckSupport.check(messageTime.equals(taraRec.timeAdvanceGrantTimes.get(0))
               && messageTime.equals(nmraRec.timeAdvanceGrantTimes.get(0)),
            "TARA or NMRA reported the wrong grant time");
         JavaTckSupport.check(flushRec.flushQueueGrantTime != null
               && flushRec.flushQueueOptimisticTime != null
               && messageTime.equals(flushRec.flushQueueOptimisticTime)
               && compareTime(flushRec.flushQueueGrantTime, initial) >= 0
               && compareTime(flushRec.flushQueueGrantTime, requestBoundary) <= 0
               && compareTime(flushRec.flushQueueOptimisticTime,
                  flushRec.flushQueueGrantTime) >= 0
               && flushRec.flushQueueGrantTime.equals(flushReceiver.queryLogicalTime()),
            "Flush Queue Grant/query returned inconsistent actual or optimistic time");
         JavaTckSupport.check(messageTime.equals(taraReceiver.queryLogicalTime())
               && messageTime.equals(nmraReceiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "logical-time queries disagreed with their requested grants");

         for (RTIambassador receiver : receivers) receiver.disableTimeConstrained();
         flushConstrained = taraConstrained = nmraConstrained = false;
         publisher.disableTimeRegulation(); publisherRegulating = false;
      } finally {
         if (flushConstrained && flushJoined) disableConstrained(flushReceiver);
         if (taraConstrained && taraJoined) disableConstrained(taraReceiver);
         if (nmraConstrained && nmraJoined) disableConstrained(nmraReceiver);
         if (publisherRegulating && publisherJoined) try {
            publisher.disableTimeRegulation();
         } catch (Exception ignored) { }
         if (flushDirectedSubscribed && flushJoined) unsubscribeDirected(flushReceiver, objectClassName);
         if (taraDirectedSubscribed && taraJoined) unsubscribeDirected(taraReceiver, objectClassName);
         if (nmraDirectedSubscribed && nmraJoined) unsubscribeDirected(nmraReceiver, objectClassName);
         if (flushAttributesSubscribed && flushJoined) unsubscribeAttributes(
            flushReceiver, objectClassName, attributeName);
         if (taraAttributesSubscribed && taraJoined) unsubscribeAttributes(
            taraReceiver, objectClassName, attributeName);
         if (nmraAttributesSubscribed && nmraJoined) unsubscribeAttributes(
            nmraReceiver, objectClassName, attributeName);
         if (directedPublished && publisherJoined) try {
            publisher.unpublishObjectClassDirectedInteractions(
               publisher.getObjectClassHandle(objectClassName));
         } catch (Exception ignored) { }
         if (attributesPublished && publisherJoined) try {
            ObjectClassHandle cls = publisher.getObjectClassHandle(objectClassName);
            publisher.unpublishObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               publisher, publisher.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (flushJoined) resign(flushReceiver);
         if (taraJoined) resign(taraReceiver);
         if (nmraJoined) resign(nmraReceiver);
         if (publisherJoined) resign(publisher);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (nmraConnected) disconnect(nmraReceiver);
         if (taraConnected) disconnect(taraReceiver);
         if (flushConnected) disconnect(flushReceiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void awaitCompletion(RTIambassador publisher, RTIambassador[] receivers,
         Recorder publisherRec, Recorder[] receiverRecs) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(publisher);
         for (RTIambassador receiver : receivers) JavaTckSupport.drain(receiver);
         if (!publisherRec.timeAdvanceGrantTimes.isEmpty()
               && !receiverRecs[0].flushQueueGrantTimes.isEmpty()
               && !receiverRecs[1].timeAdvanceGrantTimes.isEmpty()
               && !receiverRecs[2].timeAdvanceGrantTimes.isEmpty()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError("alternate directed advances did not all complete within 10 seconds");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compareTime(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   private static void checkDiscovery(Recorder recorder, ObjectInstanceHandle target,
         ObjectClassHandle objectClass, FederateHandle producer, String role) {
      JavaTckSupport.check(recorder.discoveries.size() == 1,
         role + " did not receive exactly one object discovery");
      Discovery value = recorder.discoveries.get(0);
      JavaTckSupport.check(target.equals(value.object) && objectClass.equals(value.objectClass)
            && producer.equals(value.producer), role + " received incorrect discovery metadata");
   }

   private static void checkDelivery(Recorder recorder, RTIambassador inspector,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         ObjectInstanceHandle target, byte[] expectedPayload, byte[] expectedTag,
         FederateHandle expectedProducer, LogicalTime<?, ?> expectedTime,
         MessageRetractionHandle expectedRetraction, String role) throws Exception {
      JavaTckSupport.check(recorder.interactions.size() == 1,
         role + " did not receive exactly one directed interaction");
      Interaction actual = recorder.interactions.get(0);
      JavaTckSupport.check(expectedClass.equals(actual.interactionClass)
            && target.equals(actual.target) && actual.parameters != null
            && actual.parameters.size() == 1
            && Arrays.equals(expectedPayload, actual.parameters.get(expectedParameter)),
         role + " returned the wrong interaction, target, or parameter value");
      JavaTckSupport.check(Arrays.equals(expectedTag, actual.tag)
            && expectedProducer.equals(actual.producer) && expectedTime.equals(actual.time),
         role + " changed the tag, producer, or logical timestamp");
      JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP,
         role + " returned incorrect timestamp order metadata");
      JavaTckSupport.check(expectedRetraction.equals(actual.retraction)
            && actual.transportation != null
            && !inspector.getTransportationTypeName(actual.transportation).isEmpty(),
         role + " omitted its retraction handle or resolvable transport metadata");
   }

   private static void unsubscribeDirected(RTIambassador rti, String objectClassName) {
      try { rti.unsubscribeObjectClassDirectedInteractions(rti.getObjectClassHandle(objectClassName)); }
      catch (Exception ignored) { }
   }

   private static void unsubscribeAttributes(RTIambassador rti, String className,
         String attributeName) {
      try {
         ObjectClassHandle cls = rti.getObjectClassHandle(className);
         AttributeHandle attribute = rti.getAttributeHandle(cls, attributeName);
         rti.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(rti, attribute));
      } catch (Exception ignored) { }
   }

   private static void disableConstrained(RTIambassador rti) {
      try { rti.disableTimeConstrained(); } catch (Exception ignored) { }
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private FederateHandle producer;
   }

   private static final class Interaction {
      private InteractionClassHandle interactionClass;
      private ObjectInstanceHandle target;
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
      private boolean timeRegulationEnabled, timeConstrainedEnabled;
      private final List<Discovery> discoveries = new ArrayList<>();
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private LogicalTime<?, ?> flushQueueGrantTime, flushQueueOptimisticTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled = true;
               else if ("timeConstrainedEnabled".equals(name)) timeConstrainedEnabled = true;
               else if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  Discovery event = new Discovery();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.objectClass = (ObjectClassHandle) arguments[1];
                  event.producer = (FederateHandle) arguments[2];
                  discoveries.add(event);
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  Interaction event = new Interaction();
                  event.interactionClass = (InteractionClassHandle) arguments[0];
                  event.target = (ObjectInstanceHandle) arguments[1];
                  event.parameters = ((ParameterHandleValueMap) arguments[2]).clone();
                  event.tag = ((byte[]) arguments[3]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[4];
                  event.producer = (FederateHandle) arguments[5];
                  event.time = (LogicalTime<?, ?>) arguments[6];
                  event.sentOrder = (OrderType) arguments[7];
                  event.receivedOrder = (OrderType) arguments[8];
                  event.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(event); callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               } else if ("flushQueueGrant".equals(name) && arguments != null
                     && arguments.length == 2) {
                  flushQueueGrantTime = (LogicalTime<?, ?>) arguments[0];
                  flushQueueOptimisticTime = (LogicalTime<?, ?>) arguments[1];
                  flushQueueGrantTimes.add(flushQueueGrantTime); callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed alternate-advance callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
