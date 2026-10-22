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
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.BooleanSupplier;

/** Receive-order directed receipt is selected using the target's known object class. */
final class DirectedKnownObjectClassFallbackTck {
   private static final long CALLBACK_TIMEOUT_NANOS = TimeUnit.SECONDS.toNanos(10);

   private DirectedKnownObjectClassFallbackTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseObjectName, String childObjectName, String markerName,
         String baseInteractionName, String childInteractionName,
         String baseParameterName, String childParameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-directed-known-class-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean publisherRegulating = false;
      boolean receiverConstrained = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "directed-known-class-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("directed-known-class-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle childObject = publisher.getObjectClassHandle(childObjectName);
         ObjectClassHandle baseObject = receiver.getObjectClassHandle(baseObjectName);
         ObjectClassHandle receiverChildObject = receiver.getObjectClassHandle(childObjectName);
         InteractionClassHandle sentClass = publisher.getInteractionClassHandle(childInteractionName);
         InteractionClassHandle baseClass = receiver.getInteractionClassHandle(baseInteractionName);
         InteractionClassHandle receiverChildClass = receiver.getInteractionClassHandle(childInteractionName);
         JavaTckSupport.check(!baseObject.equals(receiverChildObject)
               && !baseClass.equals(receiverChildClass)
               && childObjectName.equals(publisher.getObjectClassName(childObject))
               && baseObjectName.equals(receiver.getObjectClassName(baseObject))
               && childObjectName.equals(receiver.getObjectClassName(receiverChildObject))
               && childInteractionName.equals(publisher.getInteractionClassName(sentClass))
               && baseInteractionName.equals(receiver.getInteractionClassName(baseClass))
               && childInteractionName.equals(
                  receiver.getInteractionClassName(receiverChildClass)),
            "adapter FOM lacks the distinct directed object and interaction hierarchies");
         AttributeHandle publisherMarker = publisher.getAttributeHandle(childObject, markerName);
         AttributeHandle receiverMarker = receiver.getAttributeHandle(baseObject, markerName);
         ParameterHandle sentBaseParameter = publisher.getParameterHandle(
            sentClass, baseParameterName);
         ParameterHandle sentChildParameter = publisher.getParameterHandle(
            sentClass, childParameterName);
         ParameterHandle receivedBaseParameter = receiver.getParameterHandle(
            baseClass, baseParameterName);
         ParameterHandle receivedChildParameter = receiver.getParameterHandle(
            receiverChildClass, childParameterName);
         JavaTckSupport.check(markerName.equals(publisher.getAttributeName(
               childObject, publisherMarker))
               && markerName.equals(receiver.getAttributeName(baseObject, receiverMarker))
               && baseParameterName.equals(publisher.getParameterName(
                  sentClass, sentBaseParameter))
               && childParameterName.equals(publisher.getParameterName(
                  sentClass, sentChildParameter))
               && baseParameterName.equals(receiver.getParameterName(
                  baseClass, receivedBaseParameter))
               && childParameterName.equals(receiver.getParameterName(
                  receiverChildClass, receivedChildParameter)),
            "directed fixture member names did not round-trip locally");

         publisher.publishObjectClassAttributes(childObject,
            JavaTckSupport.attributeSet(publisher, publisherMarker));
         receiver.subscribeObjectClassAttributes(baseObject,
            JavaTckSupport.attributeSet(receiver, receiverMarker));
         publisher.publishObjectClassDirectedInteractions(childObject,
            JavaTckSupport.interactionSet(publisher, sentClass));
         receiver.subscribeObjectClassDirectedInteractionsUniversally(baseObject,
            JavaTckSupport.interactionSet(receiver, baseClass));
         receiver.subscribeObjectClassDirectedInteractionsUniversally(receiverChildObject,
            JavaTckSupport.interactionSet(receiver, receiverChildClass));

         ObjectInstanceHandle target = publisher.registerObjectInstance(childObject);
         String targetName = publisher.getObjectInstanceName(target);
         JavaTckSupport.check(targetName != null && !targetName.isEmpty(),
            "registered directed target has no object name");
         await(receiver, callbackModel,
            () -> receiverRecorder.discoveries.stream()
               .anyMatch(discovery -> targetName.equals(discovery.name)),
            "base-class discovery of the directed target");
         Discovery discovery = receiverRecorder.discoveries.stream()
            .filter(event -> targetName.equals(event.name)).findFirst().orElse(null);
         ObjectInstanceHandle receiverTarget = receiver.getObjectInstanceHandle(targetName);
         JavaTckSupport.check(receiverRecorder.discoveries.size() == 1
               && discovery != null && receiverTarget.equals(discovery.object)
               && baseObject.equals(discovery.objectClass)
               && baseObject.equals(receiver.getKnownObjectClassHandle(receiverTarget)),
            "receiver did not know the child-registered target only at the base object class");

         byte[] baseValue = new byte[] {0x42};
         byte[] childValue = new byte[] {0x43};
         byte[] tag = new byte[] {0x44, 0x4b, 0x43};
         ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory().create(2);
         parameters.put(sentBaseParameter, baseValue);
         parameters.put(sentChildParameter, childValue);
         publisher.sendDirectedInteraction(sentClass, target, parameters, tag);
         await(receiver, callbackModel,
            () -> !receiverRecorder.directed.isEmpty()
               || receiverRecorder.nonReceiveOrderDirectedCount.get() > 0,
            "receive-order directed fallback");
         pumpQuiet(receiver, callbackModel);
         JavaTckSupport.check(receiverRecorder.directed.size() == 1
               && receiverRecorder.nonReceiveOrderDirectedCount.get() == 0,
            "known-class directed subscriber did not receive exactly one callback");
         Directed delivery = receiverRecorder.directed.get(0);
         JavaTckSupport.check(baseClass.equals(delivery.interaction)
               && receiverTarget.equals(delivery.target)
               && producer.equals(delivery.producer)
               && delivery.parameters.size() == 1
               && Arrays.equals(baseValue, delivery.parameters.get(receivedBaseParameter))
               && !delivery.parameters.containsKey(receivedChildParameter)
               && Arrays.equals(tag, delivery.tag),
            "directed receipt did not select the base known-class subscription and parameter view");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "directed known-class members selected different time factories");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval lookahead = offset(epsilon, 5);
         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(receiver, callbackModel,
            () -> receiverRecorder.timeConstrainedEnabled.get() == 1,
            "directed known-class Time Constrained Enabled");
         publisher.enableTimeRegulation(lookahead);
         publisherRegulating = true;
         await(publisher, callbackModel,
            () -> publisherRecorder.timeRegulationEnabled.get() == 1,
            "directed known-class Time Regulation Enabled");
         publisher.changeInteractionOrderType(sentClass, OrderType.TIMESTAMP);

         LogicalTime publisherCurrent = offset(publisherInitial, epsilon, 2);
         publisher.timeAdvanceRequest(publisherCurrent);
         await(publisher, callbackModel,
            () -> publisherRecorder.timeAdvanceGrants.size() == 1,
            "directed known-class regulating sender advance");
         JavaTckSupport.check(publisherCurrent.equals(
               publisherRecorder.timeAdvanceGrants.get(0)),
            "regulating sender granted at the wrong logical time");
         LogicalTime messageTime = offset(publisherCurrent, epsilon, 5);
         LogicalTime requestBoundary = offset(receiverInitial, epsilon, 10);
         receiverRecorder.timedDirected.clear();
         receiverRecorder.timeAdvanceGrants.clear();
         receiverRecorder.callbackOrder.clear();
         receiver.nextMessageRequest(requestBoundary);
         pumpQuiet(receiver, callbackModel);
         JavaTckSupport.check(receiverRecorder.timedDirected.isEmpty()
               && receiverRecorder.timeAdvanceGrants.isEmpty(),
            "directed known-class NMR completed before its future message arrived");
         MessageRetractionReturn sent = publisher.sendDirectedInteraction(
            sentClass, target, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "timestamped directed known-class send returned no retraction handle");
         await(receiver, callbackModel,
            () -> receiverRecorder.timedDirected.size() == 1
               && receiverRecorder.timeAdvanceGrants.size() == 1,
            "timestamped directed known-class NMR delivery and grant");
         pumpQuiet(receiver, callbackModel);
         TimedDirected timed = receiverRecorder.timedDirected.get(0);
         JavaTckSupport.check(receiverRecorder.timedDirected.size() == 1
               && receiverRecorder.nonReceiveOrderDirectedCount.get() == 0
               && baseClass.equals(timed.delivery.interaction)
               && receiverTarget.equals(timed.delivery.target)
               && producer.equals(timed.delivery.producer)
               && timed.delivery.parameters.size() == 1
               && Arrays.equals(baseValue,
                  timed.delivery.parameters.get(receivedBaseParameter))
               && !timed.delivery.parameters.containsKey(receivedChildParameter)
               && Arrays.equals(tag, timed.delivery.tag)
               && messageTime.equals(timed.time)
               && timed.sentOrder == OrderType.TIMESTAMP
               && timed.receivedOrder == OrderType.TIMESTAMP
               && timed.retraction != null && timed.retraction.encodedLength() > 0
               && messageTime.equals(receiverRecorder.timeAdvanceGrants.get(0))
               && receiverRecorder.callbackOrder.equals(Arrays.asList("directed", "grant"))
               && receiverRecorder.directed.size() == 1,
            "timestamped directed receipt did not preserve known-class projection and NMR order");

         receiver.disableTimeConstrained();
         receiverConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;

         receiver.unsubscribeObjectClassDirectedInteractions(receiverChildObject,
            JavaTckSupport.interactionSet(receiver, receiverChildClass));
         receiver.unsubscribeObjectClassDirectedInteractions(baseObject,
            JavaTckSupport.interactionSet(receiver, baseClass));
         receiver.unsubscribeObjectClassAttributes(baseObject,
            JavaTckSupport.attributeSet(receiver, receiverMarker));
         publisher.unpublishObjectClassDirectedInteractions(childObject,
            JavaTckSupport.interactionSet(publisher, sentClass));
         publisher.unpublishObjectClassAttributes(childObject,
            JavaTckSupport.attributeSet(publisher, publisherMarker));
      } finally {
         if (receiverJoined && receiverConstrained) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherJoined && publisherRegulating) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
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

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
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

   private static void pumpQuiet(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(150);
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.sleep(1);
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String name) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
      }
   }

   private static final class Directed {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle target;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;

      private Directed(InteractionClassHandle interaction, ObjectInstanceHandle target,
            ParameterHandleValueMap parameters, byte[] tag, FederateHandle producer) {
         this.interaction = interaction;
         this.target = target;
         this.parameters = parameters;
         this.tag = tag;
         this.producer = producer;
      }
   }

   private static final class TimedDirected {
      private final Directed delivery;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private TimedDirected(Directed delivery, LogicalTime<?, ?> time,
            OrderType sentOrder, OrderType receivedOrder,
            MessageRetractionHandle retraction) {
         this.delivery = delivery;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
         this.retraction = retraction;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Directed> directed = new CopyOnWriteArrayList<>();
      private final List<TimedDirected> timedDirected = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final AtomicInteger nonReceiveOrderDirectedCount = new AtomicInteger();
      private final AtomicInteger timeConstrainedEnabled = new AtomicInteger();
      private final AtomicInteger timeRegulationEnabled = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled.incrementAndGet();
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled.incrementAndGet();
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               } else if ("discoverObjectInstance".equals(name)) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2]));
               } else if ("receiveDirectedInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  directed.add(new Directed((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1],
                     ((ParameterHandleValueMap) arguments[2]).clone(),
                     ((byte[]) arguments[3]).clone(), (FederateHandle) arguments[5]));
               } else if ("receiveDirectedInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Directed delivery = new Directed((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1],
                     ((ParameterHandleValueMap) arguments[2]).clone(),
                     ((byte[]) arguments[3]).clone(), (FederateHandle) arguments[5]);
                  timedDirected.add(new TimedDirected(delivery,
                     (LogicalTime<?, ?>) arguments[6], (OrderType) arguments[7],
                     (OrderType) arguments[8], (MessageRetractionHandle) arguments[9]));
                  callbackOrder.add("directed");
               } else if ("receiveDirectedInteraction".equals(name)) {
                  nonReceiveOrderDirectedCount.incrementAndGet();
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK known-class recorder";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
