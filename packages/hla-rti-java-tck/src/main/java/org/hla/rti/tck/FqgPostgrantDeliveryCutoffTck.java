package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
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

/** Checks the strict post-FQG TSO cutoff and a later deliverable timestamp. */
final class FqgPostgrantDeliveryCutoffTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private FqgPostgrantDeliveryCutoffTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador firstPublisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador latePublisher = factory.getRtiAmbassador();
      Recorder firstCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      Recorder lateCallbacks = new Recorder();
      String federation = "java-tck-fqg-cutoff-" + UUID.randomUUID();
      boolean firstConnected = false;
      boolean receiverConnected = false;
      boolean lateConnected = false;
      boolean federationCreated = false;
      boolean firstJoined = false;
      boolean receiverJoined = false;
      boolean lateJoined = false;
      try {
         firstPublisher.connect(firstCallbacks.proxy(), callbackModel);
         firstConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         latePublisher.connect(lateCallbacks.proxy(), callbackModel);
         lateConnected = true;
         firstPublisher.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         FederateHandle firstFederateHandle = firstPublisher.joinFederationExecution(
            "java-tck-fqg-first-publisher", federation);
         firstJoined = true;
         JavaTckSupport.check(firstFederateHandle != null,
            "FQG first publisher join returned no federate handle");
         receiver.joinFederationExecution("java-tck-fqg-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle firstClass = firstPublisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle receiverClass = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle firstParameter = firstPublisher.getParameterHandle(firstClass, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverClass, parameterName);
         JavaTckSupport.check(firstClass != null && receiverClass != null
               && firstParameter != null && receiverParameter != null
               && firstClass.equals(receiverClass) && firstParameter.equals(receiverParameter),
            "FQG adapter FOM did not resolve stable interaction handles");
         firstPublisher.publishInteractionClass(firstClass);
         firstPublisher.changeInteractionOrderType(firstClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);

         LogicalTimeFactory<?, ?> firstFactory = firstPublisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(firstFactory.getName().equals(receiverFactory.getName()),
            "FQG initial members selected different logical-time factories");
         LogicalTime<?, ?> firstInitial = firstPublisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(firstInitial.equals(receiverInitial),
            "FQG initial members began at different logical times");
         LogicalTimeInterval firstEpsilon = (LogicalTimeInterval) firstFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon = (LogicalTimeInterval) receiverFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.timeConstrainedEnabled == 1,
            "FQG receiver Time Constrained callback");
         LogicalTimeInterval firstLookahead = offset(firstEpsilon, 5);
         firstPublisher.enableTimeRegulation(firstLookahead);
         await(firstPublisher, callbackModel,
            () -> firstCallbacks.timeRegulationEnabled == 1,
            "FQG first publisher Time Regulation callback");

         LogicalTime<?, ?> timestampSeven = offset(firstInitial, firstEpsilon, 7);
         LogicalTime<?, ?> firstTargetTwo = offset(firstInitial, firstEpsilon, 2);
         LogicalTime<?, ?> requestBoundary = offset(receiverInitial, receiverEpsilon, 10);
         byte[] firstPayload = {0x46, 0x51, 0x47, 0x2d, 0x37};
         byte[] firstTag = {0x46, 0x51, 0x47, 0x2d, 0x37};
         send(firstPublisher, firstClass, firstParameter,
            firstPayload, firstTag, timestampSeven, "FQG initial TSO");
         JavaTckSupport.check(receiverCallbacks.interactions.isEmpty(),
            "FQG initial TSO was delivered before a request");
         receiverCallbacks.callbackOrder.clear();
         firstPublisher.timeAdvanceRequest(firstTargetTwo);
         await(firstPublisher, callbackModel,
            () -> firstCallbacks.timeAdvanceGrants.size() == 1,
            "FQG first publisher TAR(2)");
         receiver.flushQueueRequest(requestBoundary);
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() == 1
               && receiverCallbacks.flushQueueGrants.size() == 1,
            "FQG initial interaction and Flush Queue grant");
         FlushGrant firstGrant = receiverCallbacks.flushQueueGrants.get(0);
         JavaTckSupport.check(receiverCallbacks.callbackOrder.equals(
               Arrays.asList("interaction", "flushQueueGrant")),
            "FQG delivered the initial queued interaction after its grant");
         checkDelivery(receiverCallbacks.interactions.get(0), receiverClass,
            receiverParameter, firstPayload, firstTag,
            firstFederateHandle, timestampSeven, "FQG initial TSO");
         JavaTckSupport.check(timestampSeven.equals(firstGrant.actual),
            "FQG returned the wrong initial actual grant time");

         FederateHandle lateFederateHandle = latePublisher.joinFederationExecution(
            "java-tck-fqg-late-publisher", federation);
         lateJoined = true;
         InteractionClassHandle lateClass = latePublisher.getInteractionClassHandle(interactionName);
         ParameterHandle lateParameter = latePublisher.getParameterHandle(lateClass, parameterName);
         JavaTckSupport.check(lateClass != null && lateParameter != null
               && lateClass.equals(receiverClass) && lateParameter.equals(receiverParameter),
            "FQG late publisher did not resolve the adapter interaction handles");
         latePublisher.publishInteractionClass(lateClass);
         latePublisher.changeInteractionOrderType(lateClass, OrderType.TIMESTAMP);
         LogicalTimeFactory<?, ?> lateFactory = latePublisher.getTimeFactory();
         JavaTckSupport.check(firstFactory.getName().equals(lateFactory.getName()),
            "FQG late publisher selected a different logical-time factory");
         LogicalTime<?, ?> lateInitial = latePublisher.queryLogicalTime();
         JavaTckSupport.check(firstInitial.equals(lateInitial),
            "FQG late publisher did not join at the initial logical time");
         LogicalTimeInterval lateEpsilon = (LogicalTimeInterval) lateFactory.makeEpsilon();
         latePublisher.enableTimeRegulation(lateEpsilon);
         await(latePublisher, callbackModel,
            () -> lateCallbacks.timeRegulationEnabled == 1,
            "FQG late publisher Time Regulation callback");

         LogicalTime<?, ?> timestampSix = offset(lateInitial, lateEpsilon, 6);
         LogicalTime<?, ?> timestampEight = offset(lateInitial, lateEpsilon, 8);
         byte[] belowGrantPayload = {0x46, 0x51, 0x47, 0x2d, 0x36};
         byte[] laterPayload = {0x46, 0x51, 0x47, 0x2d, 0x38};
         byte[] belowGrantTag = {0x46, 0x51, 0x47, 0x2d, 0x36};
         byte[] laterTag = {0x46, 0x51, 0x47, 0x2d, 0x38};
         send(latePublisher, lateClass, lateParameter, belowGrantPayload,
            belowGrantTag, timestampSix, "FQG below-grant TSO");
         send(latePublisher, lateClass, lateParameter,
            laterPayload, laterTag, timestampEight, "FQG later TSO");
         JavaTckSupport.check(receiverCallbacks.interactions.size() == 1,
            "FQG delivered a late interaction before the receiver advanced again");

         advanceAndWait(latePublisher, lateCallbacks, callbackModel,
            offset(lateInitial, lateEpsilon, 2), 1, "FQG late publisher TAR(2)");
         advanceAndWait(firstPublisher, firstCallbacks, callbackModel,
            offset(firstInitial, firstEpsilon, 3), 2, "FQG first publisher TAR(3)");
         advanceAndWait(latePublisher, lateCallbacks, callbackModel,
            offset(lateInitial, lateEpsilon, 4), 2, "FQG late publisher TAR(4)");
         advanceAndWait(firstPublisher, firstCallbacks, callbackModel,
            offset(firstInitial, firstEpsilon, 5), 3, "FQG first publisher TAR(5)");
         advanceAndWait(latePublisher, lateCallbacks, callbackModel,
            offset(lateInitial, lateEpsilon, 6), 3, "FQG late publisher TAR(6)");
         advanceAndWait(firstPublisher, firstCallbacks, callbackModel,
            offset(firstInitial, firstEpsilon, 7), 4, "FQG first publisher TAR(7)");
         advanceAndWait(latePublisher, lateCallbacks, callbackModel,
            offset(lateInitial, lateEpsilon, 8), 4, "FQG late publisher TAR(8)");
         LogicalTime<?, ?> firstTargetEight = offset(firstInitial, firstEpsilon, 8);
         advanceAndWait(firstPublisher, firstCallbacks, callbackModel,
            firstTargetEight, 5, "FQG first publisher TAR(8)");
         advanceAndWait(latePublisher, lateCallbacks, callbackModel,
            offset(lateInitial, lateEpsilon, 10), 5, "FQG late publisher TAR(10)");

         LogicalTime<?, ?> receiverTargetNine = offset(receiverInitial, receiverEpsilon, 9);
         receiver.timeAdvanceRequestAvailable(receiverTargetNine);
         await(receiver, callbackModel,
            () -> receiverCallbacks.timeAdvanceGrants.size() == 1
               && receiverCallbacks.interactions.size() == 2,
            "FQG receiver TARA grant and later interaction");
         LogicalTime<?, ?> expectedReceiverTime = offset(receiverInitial, receiverEpsilon, 8);
         LogicalTime<?, ?> queriedReceiverTime = receiver.queryLogicalTime();
         JavaTckSupport.check(firstTargetEight.equals(
               firstCallbacks.timeAdvanceGrants.get(4))
               && offset(lateInitial, lateEpsilon, 10).equals(
                  lateCallbacks.timeAdvanceGrants.get(4)),
            "FQG regulating producers did not reach their requested final times");
         checkDelivery(receiverCallbacks.interactions.get(1), receiverClass,
            receiverParameter, laterPayload, laterTag, lateFederateHandle,
            timestampEight, "FQG later legal TSO");
         JavaTckSupport.check(expectedReceiverTime.equals(queriedReceiverTime)
               && receiverCallbacks.timeAdvanceGrants.get(0).equals(expectedReceiverTime),
            "FQG receiver advanced past the next deliverable timestamp");
         JavaTckSupport.check(receiverCallbacks.interactions.size() == 2
               && !receiverCallbacks.interactions.stream()
                  .anyMatch(delivery -> timestampSix.equals(delivery.time))
               && receiverCallbacks.callbackOrder.equals(Arrays.asList(
                  "interaction", "flushQueueGrant", "interaction", "timeAdvanceGrant")),
            "FQG delivered a TSO below the grant or returned callbacks out of order");

         latePublisher.resignFederationExecution(ResignAction.NO_ACTION);
         lateJoined = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         firstPublisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         firstJoined = false;
         firstPublisher.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (lateJoined) resign(latePublisher);
         if (receiverJoined) resign(receiver);
         if (firstJoined) resign(firstPublisher);
         if (federationCreated) {
            try { firstPublisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (lateConnected) disconnect(latePublisher);
         if (receiverConnected) disconnect(receiver);
         if (firstConnected) disconnect(firstPublisher);
      }
   }

   private static void send(RTIambassador ambassador, InteractionClassHandle interaction,
         ParameterHandle parameter, byte[] payload, byte[] tag, LogicalTime<?, ?> time,
         String description) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload.clone());
      hla.rti1516_2025.MessageRetractionReturn result =
         ambassador.sendInteraction(interaction, values, tag.clone(), time);
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null, description + " returned no valid retraction handle");
   }

   private static void checkDelivery(Delivery delivery, InteractionClassHandle expectedClass,
         ParameterHandle expectedParameter, byte[] expectedPayload, byte[] expectedTag,
         FederateHandle expectedProducer, LogicalTime<?, ?> expectedTime, String description) {
      JavaTckSupport.check(expectedClass.equals(delivery.interaction)
            && delivery.parameters.size() == 1
            && Arrays.equals(expectedPayload, delivery.parameters.get(expectedParameter)),
         description + " returned the wrong interaction or parameter payload");
      JavaTckSupport.check(Arrays.equals(expectedTag, delivery.tag)
            && expectedProducer.equals(delivery.producer)
            && expectedTime.equals(delivery.time)
            && delivery.sentOrder == OrderType.TIMESTAMP
            && delivery.receivedOrder == OrderType.TIMESTAMP,
         description + " changed its tag, producer, timestamp, or order metadata");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval offset(LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTimeInterval result = interval;
      for (int index = 1; index < steps; ++index) result = (LogicalTimeInterval) result.add(interval);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void advanceAndWait(RTIambassador ambassador, Recorder callbacks,
         CallbackModel model, LogicalTime<?, ?> target, int expectedGrantCount, String description)
         throws Exception {
      ambassador.timeAdvanceRequest(target);
      await(ambassador, model,
         () -> callbacks.timeAdvanceGrants.size() == expectedGrantCount, description);
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      awaitAll(new RTIambassador[] {ambassador}, model, condition, description);
   }

   private static void awaitAll(RTIambassador[] ambassadors, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) ambassador.evokeCallback(0.0);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Delivery {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;

      private Delivery(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
      }
   }

   private static final class FlushGrant {
      private final LogicalTime<?, ?> actual;
      private FlushGrant(LogicalTime<?, ?> actual) {
         this.actual = actual;
      }
   }

   private static final class Recorder {
      private volatile int timeConstrainedEnabled;
      private volatile int timeRegulationEnabled;
      private final List<Delivery> interactions = new CopyOnWriteArrayList<>();
      private final List<FlushGrant> flushQueueGrants = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new CopyOnWriteArrayList<>();
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
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  interactions.add(new Delivery(arguments));
                  callbackOrder.add("interaction");
               } else if ("flushQueueGrant".equals(name)
                     && arguments != null && arguments.length == 2) {
                  flushQueueGrants.add(new FlushGrant((LogicalTime<?, ?>) arguments[0]));
                  callbackOrder.add("flushQueueGrant");
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("timeAdvanceGrant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK FQG cutoff callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
