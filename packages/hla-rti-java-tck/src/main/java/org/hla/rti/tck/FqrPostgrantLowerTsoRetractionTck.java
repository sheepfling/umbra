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

/** Checks FQR's optimistic flush, later lower TSO, and retraction callback. */
final class FqrPostgrantLowerTsoRetractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;
   private static final long EARLY_CALLBACK_WINDOW_NANOS = 50_000_000L;

   private FqrPostgrantLowerTsoRetractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-fqr-postgrant-lower-tso-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean federationCreated = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-fqr-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-fqr-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherClass =
            publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle receiverClass =
            receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherClass, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverClass, parameterName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherParameter != null && receiverParameter != null
               && publisherClass.equals(receiverClass)
               && publisherParameter.equals(receiverParameter),
            "FQR adapter FOM did not resolve stable interaction handles");
         publisher.publishInteractionClass(publisherClass);
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "FQR members selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "FQR members began at different logical times");
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.timeConstrainedEnabled == 1,
            "FQR receiver Time Constrained callback");
         LogicalTimeInterval lookahead = offset(publisherEpsilon, 5);
         publisher.enableTimeRegulation(lookahead);
         await(publisher, callbackModel,
            () -> publisherCallbacks.timeRegulationEnabled == 1,
            "FQR publisher Time Regulation callback");
         JavaTckSupport.check(lookahead.equals(publisher.queryLookahead()),
            "FQR publisher did not retain its configured lookahead");

         LogicalTime<?, ?> firstTime = offset(publisherInitial, publisherEpsilon, 12);
         LogicalTime<?, ?> rejectedTime = offset(publisherInitial, publisherEpsilon, 4);
         LogicalTime<?, ?> laterLowerTime = offset(publisherInitial, publisherEpsilon, 6);
         LogicalTime<?, ?> requestBoundary = offset(receiverInitial, receiverEpsilon, 10);
         LogicalTime<?, ?> expectedActualGrant = offset(receiverInitial, receiverEpsilon, 5);
         byte[] firstPayload = {0x46, 0x51, 0x52, 0x2d, 0x31};
         byte[] laterPayload = {0x46, 0x51, 0x52, 0x2d, 0x32};
         byte[] firstTag = {0x46, 0x51, 0x52, 0x2d, 0x31};
         byte[] laterTag = {0x46, 0x51, 0x52, 0x2d, 0x32};

         MessageRetractionReturn firstRetraction = publisher.sendInteraction(
            publisherClass, oneParameter(publisher, publisherParameter, firstPayload),
            firstTag.clone(), firstTime);
         checkRetraction(firstRetraction, "FQR future interaction");
         pumpUnexpectedCallbacks(receiver, callbackModel, receiverCallbacks);
         JavaTckSupport.check(receiverCallbacks.interactions.isEmpty(),
            "FQR future interaction was delivered before Flush Queue Request");
         receiverCallbacks.callbackOrder.clear();
         receiver.flushQueueRequest(requestBoundary);
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() == 1
               && receiverCallbacks.flushQueueGrants.size() == 1,
            "FQR first future interaction and grant");

         Interaction firstDelivery = receiverCallbacks.interactions.get(0);
         FlushQueueGrant firstGrant = receiverCallbacks.flushQueueGrants.get(0);
         JavaTckSupport.check(receiverCallbacks.callbackOrder.equals(
               Arrays.asList("interaction", "flushQueueGrant")),
            "FQR delivered its first queued interaction after the grant");
         checkInteraction(firstDelivery, receiverClass, receiverParameter, firstPayload,
            firstTag, producer, firstTime, firstRetraction.handle,
            "FQR first future interaction");
         JavaTckSupport.check(expectedActualGrant.equals(firstGrant.actual)
               && requestBoundary.equals(firstGrant.optimistic)
               && expectedActualGrant.equals(receiver.queryLogicalTime()),
            "FQR reported or queried the wrong actual/optimistic frontier");
         LogicalTime<?, ?> belowOptimisticFloor =
            offset(expectedActualGrant, receiverEpsilon, 1);
         JavaTckSupport.check(compareTime(belowOptimisticFloor, expectedActualGrant) > 0
               && compareTime(belowOptimisticFloor, requestBoundary) < 0,
            "FQR advance probe did not distinguish actual and optimistic time");
         JavaTckSupport.expectFailure(
            () -> receiver.timeAdvanceRequest(belowOptimisticFloor),
            "LogicalTimeAlreadyPassed",
            "timeAdvanceRequest after FQG below its optimistic logical time");
         JavaTckSupport.expectFailure(
            () -> receiver.timeAdvanceRequestAvailable(belowOptimisticFloor),
            "LogicalTimeAlreadyPassed",
            "timeAdvanceRequestAvailable after FQG below its optimistic logical time");
         JavaTckSupport.expectFailure(
            () -> receiver.nextMessageRequest(belowOptimisticFloor),
            "LogicalTimeAlreadyPassed",
            "nextMessageRequest after FQG below its optimistic logical time");
         JavaTckSupport.expectFailure(
            () -> receiver.nextMessageRequestAvailable(belowOptimisticFloor),
            "LogicalTimeAlreadyPassed",
            "nextMessageRequestAvailable after FQG below its optimistic logical time");
         JavaTckSupport.expectFailure(
            () -> receiver.flushQueueRequest(belowOptimisticFloor),
            "LogicalTimeAlreadyPassed",
            "flushQueueRequest after FQG below its optimistic logical time");
         JavaTckSupport.check(expectedActualGrant.equals(receiver.queryLogicalTime()),
            "rejected post-FQG advance changed the actual logical time");

         JavaTckSupport.expectFailure(
            () -> publisher.sendInteraction(
               publisherClass,
               oneParameter(publisher, publisherParameter, firstPayload),
               firstTag.clone(),
               rejectedTime),
            "InvalidLogicalTime",
            "timestamped interaction below current time plus lookahead after FQR");
         JavaTckSupport.check(receiverCallbacks.interactions.size() == 1,
            "rejected FQR input produced an unexpected receiver callback");

         MessageRetractionReturn laterRetraction = publisher.sendInteraction(
            publisherClass, oneParameter(publisher, publisherParameter, laterPayload),
            laterTag.clone(), laterLowerTime);
         checkRetraction(laterRetraction, "FQR later lower TSO");
         receiverCallbacks.callbackOrder.clear();
         receiver.flushQueueRequest(requestBoundary);
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() == 2
               && receiverCallbacks.flushQueueGrants.size() == 2,
            "FQR later lower TSO and second grant");
         Interaction laterDelivery = receiverCallbacks.interactions.get(1);
         JavaTckSupport.check(firstTime.equals(firstDelivery.time)
               && laterLowerTime.equals(laterDelivery.time)
               && receiverCallbacks.callbackOrder.equals(
                  Arrays.asList("interaction", "flushQueueGrant")),
            "FQR did not deliver the later lower TSO before its second grant");
         checkInteraction(laterDelivery, receiverClass, receiverParameter, laterPayload,
            laterTag, producer, laterLowerTime, laterRetraction.handle,
            "FQR later lower TSO");

         receiverCallbacks.retractions.clear();
         publisher.retract(firstRetraction.handle);
         await(receiver, callbackModel,
            () -> receiverCallbacks.retractions.size() == 1,
            "FQR Request Retraction callback");
         JavaTckSupport.check(firstRetraction.handle.equals(
               receiverCallbacks.retractions.get(0)),
            "FQR Request Retraction callback did not identify the previously delivered TSO");

         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (federationCreated) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static ParameterHandleValueMap oneParameter(RTIambassador ambassador,
         ParameterHandle parameter, byte[] payload) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload.clone());
      return values;
   }

   private static void checkRetraction(MessageRetractionReturn result, String description) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null, description + " returned no valid retraction handle");
   }

   private static void checkInteraction(Interaction interaction,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         byte[] expectedPayload, byte[] expectedTag, FederateHandle expectedProducer,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         String description) {
      JavaTckSupport.check(expectedClass.equals(interaction.interactionClass)
            && interaction.parameters.size() == 1
            && Arrays.equals(expectedPayload, interaction.parameters.get(expectedParameter)),
         description + " returned the wrong interaction or payload");
      JavaTckSupport.check(Arrays.equals(expectedTag, interaction.tag)
            && expectedProducer.equals(interaction.producer),
         description + " changed its tag or producer");
      JavaTckSupport.check(expectedTime.equals(interaction.time)
            && interaction.sentOrder == OrderType.TIMESTAMP
            && interaction.receivedOrder == OrderType.TIMESTAMP
            && expectedRetraction.equals(interaction.retraction),
         description + " changed its timestamp, order, or retraction metadata");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval offset(LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTimeInterval result = interval;
      for (int index = 1; index < steps; ++index) {
         result = (LogicalTimeInterval) result.add(interval);
      }
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compareTime(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pumpUnexpectedCallbacks(RTIambassador ambassador, CallbackModel model,
         Recorder callbacks) throws Exception {
      long deadline = System.nanoTime() + EARLY_CALLBACK_WINDOW_NANOS;
      while (callbacks.interactions.isEmpty() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Interaction {
      private final InteractionClassHandle interactionClass;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Interaction(Object[] arguments) {
         interactionClass = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class FlushQueueGrant {
      private final LogicalTime<?, ?> actual;
      private final LogicalTime<?, ?> optimistic;

      private FlushQueueGrant(LogicalTime<?, ?> actual, LogicalTime<?, ?> optimistic) {
         this.actual = actual;
         this.optimistic = optimistic;
      }
   }

   private static final class Recorder {
      private volatile int timeConstrainedEnabled;
      private volatile int timeRegulationEnabled;
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<FlushQueueGrant> flushQueueGrants = new CopyOnWriteArrayList<>();
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled++;
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  interactions.add(new Interaction(arguments));
                  callbackOrder.add("interaction");
               } else if ("flushQueueGrant".equals(name) && arguments != null
                     && arguments.length == 2) {
                  flushQueueGrants.add(new FlushQueueGrant(
                     (LogicalTime<?, ?>) arguments[0], (LogicalTime<?, ?>) arguments[1]));
                  callbackOrder.add("flushQueueGrant");
               } else if ("requestRetraction".equals(name) && arguments != null
                     && arguments.length == 1) {
                  retractions.add((MessageRetractionHandle) arguments[0]);
                  callbackOrder.add("requestRetraction");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK FQR post-grant callbacks";
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
