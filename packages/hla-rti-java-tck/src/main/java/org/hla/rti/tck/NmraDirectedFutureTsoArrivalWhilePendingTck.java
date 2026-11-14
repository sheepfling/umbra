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
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
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

/** Checks a directed TSO arriving while an NMRA request is pending. */
final class NmraDirectedFutureTsoArrivalWhilePendingTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long PENDING_WINDOW_NANOS = 40_000_000L;

   private NmraDirectedFutureTsoArrivalWhilePendingTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "NMRA directed callback-order assertions require HLA_EVOKED callbacks");
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder publisherCallbacks = new Recorder();
      String federation = "java-tck-nmra-directed-future-arrival-" + UUID.randomUUID();
      String publisherName = "java-tck-nmra-directed-publisher";
      boolean requesterConnected = false;
      boolean publisherConnected = false;
      boolean federationCreated = false;
      boolean requesterJoined = false;
      boolean publisherJoined = false;
      boolean requesterSubscribed = false;
      boolean publisherPublished = false;
      ObjectClassHandle requesterClass = null;
      ObjectClassHandle publisherClass = null;
      AttributeHandle requesterAttribute = null;
      AttributeHandle publisherAttribute = null;
      InteractionClassHandle requesterInteraction = null;
      InteractionClassHandle publisherInteraction = null;
      try {
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         requester.joinFederationExecution(
            "java-tck-nmra-directed-requester", "requester", federation);
         requesterJoined = true;
         publisher.joinFederationExecution(publisherName, "publisher", federation);
         publisherJoined = true;

         requesterClass = requester.getObjectClassHandle(objectClassName);
         publisherClass = publisher.getObjectClassHandle(objectClassName);
         requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         requesterInteraction = requester.getInteractionClassHandle(interactionName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(requesterClass != null && publisherClass != null
               && requesterAttribute != null && publisherAttribute != null
               && requesterInteraction != null && publisherInteraction != null
               && requesterClass.equals(publisherClass)
               && requesterAttribute.equals(publisherAttribute)
               && requesterInteraction.equals(publisherInteraction),
            "NMRA directed FOM handles did not retain identity across members");

         AttributeHandleSet requesterAttributes =
            JavaTckSupport.attributeSet(requester, requesterAttribute);
         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         InteractionClassHandleSet requesterDirected =
            JavaTckSupport.interactionSet(requester, requesterInteraction);
         InteractionClassHandleSet publisherDirected =
            JavaTckSupport.interactionSet(publisher, publisherInteraction);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         requester.subscribeObjectClassDirectedInteractionsUniversally(
            requesterClass, requesterDirected);
         requesterSubscribed = true;
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisher.publishObjectClassDirectedInteractions(publisherClass, publisherDirected);
         publisherPublished = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> requesterTimeFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(requesterTimeFactory.getName().equals(publisherTimeFactory.getName()),
            "NMRA directed members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(publisherInitial),
            "NMRA directed members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterTimeFactory.makeEpsilon();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(requester, () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "NMRA directed requester Time Constrained callback");
         publisher.enableTimeRegulation(publisherEpsilon);
         await(publisher, () -> publisherCallbacks.timeRegulationEnabled == 1,
            "NMRA directed publisher Time Regulation callback");

         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> messageTime = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime<?, ?> publisherTarget = offset(publisherInitial, publisherEpsilon, 4);
         requester.nextMessageRequestAvailable(requestTime);
         requirePending(requester, requesterCallbacks);

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         await(requester, () -> !requesterCallbacks.discoveries.isEmpty(),
            "NMRA directed target discovery");
         Discovery discovery = requesterCallbacks.discoveries.get(0);
         JavaTckSupport.check(requesterCallbacks.discoveries.size() == 1
               && target.equals(discovery.object)
               && requesterClass.equals(discovery.objectClass),
            "NMRA receiver discovered a different directed target object or class");

         byte[] tag = {0x4e, 0x4d, 0x52, 0x41, 0x44};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(0);
         MessageRetractionReturn sent = publisher.sendDirectedInteraction(
            publisherInteraction, target, parameters, tag.clone(), messageTime);
         JavaTckSupport.check(sent != null,
            "NMRA directed future-arrival send returned no result");
         boolean validSendRetraction = sent.retractionHandleIsValid && sent.handle != null;
         requirePending(requester, requesterCallbacks);
         publisher.timeAdvanceRequest(publisherTarget);
         awaitBoth(requester, publisher,
            () -> requesterCallbacks.directed.size() == 1
               && requesterCallbacks.grants.size() == 1
               && publisherCallbacks.grants.size() == 1,
            "NMRA directed future-arrival delivery and grant");

         JavaTckSupport.check(requesterCallbacks.directed.size() == 1,
            "NMRA delivered an unexpected number of directed TSO callbacks");
         DirectedInteraction delivered = requesterCallbacks.directed.get(0);
         FederateHandle expectedProducer = requester.getFederateHandle(publisherName);
         JavaTckSupport.check(expectedProducer != null
               && target.equals(delivered.object)
               && requesterInteraction.equals(delivered.interaction)
               && expectedProducer.equals(delivered.producer)
               && delivered.parameters.isEmpty()
               && Arrays.equals(tag, delivered.tag)
               && messageTime.equals(delivered.time)
               && delivered.sentOrder == OrderType.TIMESTAMP
               && delivered.receivedOrder == OrderType.TIMESTAMP
               && delivered.retraction != null,
            "NMRA changed or duplicated the timestamped directed interaction");
         JavaTckSupport.check(validSendRetraction,
            "NMRA directed future-arrival send returned no valid retraction handle (valid="
               + sent.retractionHandleIsValid + ", handle=" + sent.handle + ")");
         JavaTckSupport.check(requesterCallbacks.grants.size() == 1
               && messageTime.equals(requesterCallbacks.grants.get(0))
               && publisherCallbacks.grants.size() == 1
               && publisherTarget.equals(publisherCallbacks.grants.get(0)),
            "NMRA directed future-arrival granted at the wrong time or more than once");
         int directedIndex = requesterCallbacks.callbackOrder.indexOf("directed");
         int grantIndex = requesterCallbacks.callbackOrder.indexOf("grant");
         JavaTckSupport.check(directedIndex >= 0 && grantIndex >= 0 && directedIndex < grantIndex,
            "directed interaction callback did not precede the NMRA grant");
      } finally {
         if (requesterJoined && requesterSubscribed && requesterClass != null
               && requesterAttribute != null && requesterInteraction != null) {
            try {
               requester.unsubscribeObjectClassDirectedInteractions(requesterClass,
                  JavaTckSupport.interactionSet(requester, requesterInteraction));
               requester.unsubscribeObjectClassAttributes(requesterClass,
                  JavaTckSupport.attributeSet(requester, requesterAttribute));
            } catch (Exception ignored) { }
         }
         if (publisherJoined && publisherPublished && publisherClass != null
               && publisherAttribute != null && publisherInteraction != null) {
            try {
               publisher.unpublishObjectClassDirectedInteractions(publisherClass,
                  JavaTckSupport.interactionSet(publisher, publisherInteraction));
               publisher.unpublishObjectClassAttributes(publisherClass,
                  JavaTckSupport.attributeSet(publisher, publisherAttribute));
            } catch (Exception ignored) { }
         }
         if (requesterJoined) resign(requester);
         if (publisherJoined) resign(publisher);
         if (federationCreated) {
            try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void requirePending(RTIambassador requester, Recorder callbacks)
         throws Exception {
      long deadline = System.nanoTime() + PENDING_WINDOW_NANOS;
      while (System.nanoTime() < deadline) {
         requester.evokeCallback(0.0);
         JavaTckSupport.check(callbacks.directed.isEmpty() && callbacks.grants.isEmpty(),
            "NMRA completed before a timestamped directed interaction arrived");
         Thread.sleep(1);
      }
   }

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         first.evokeCallback(0.0);
         second.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;

      private Discovery(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         objectClass = (ObjectClassHandle) arguments[1];
      }
   }

   private static final class DirectedInteraction {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle object;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private DirectedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         object = (ObjectInstanceHandle) arguments[1];
         parameters = ((ParameterHandleValueMap) arguments[2]).clone();
         tag = ((byte[]) arguments[3]).clone();
         producer = (FederateHandle) arguments[5];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile int timeConstrainedEnabled;
      private volatile int timeRegulationEnabled;
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<DirectedInteraction> directed = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               Object[] args = arguments == null ? new Object[0] : arguments;
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled++;
               } else if ("discoverObjectInstance".equals(name) && args.length >= 2) {
                  discoveries.add(new Discovery(args));
               } else if ("receiveDirectedInteraction".equals(name) && args.length == 10) {
                  directed.add(new DirectedInteraction(args));
                  callbackOrder.add("directed");
               } else if ("timeAdvanceGrant".equals(name) && args.length == 1) {
                  grants.add((LogicalTime<?, ?>) args[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK NMRA directed callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args.length == 0 ? null : args[0]);
               }
               return null;
            });
      }
   }
}
