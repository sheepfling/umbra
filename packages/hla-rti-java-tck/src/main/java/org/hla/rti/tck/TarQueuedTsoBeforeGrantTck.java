package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
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

final class TarQueuedTsoBeforeGrantTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;
   private static final long EARLY_CALLBACK_WINDOW_NANOS = 50_000_000L;

   private TarQueuedTsoBeforeGrantTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-tar-queued-before-grant-" + UUID.randomUUID();
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
            "java-tck-tar-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-tar-receiver", federation);
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
            "TAR adapter FOM did not resolve stable interaction handles");
         publisher.publishInteractionClass(publisherClass);
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "TAR members selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "TAR members began at different logical times");
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.timeConstrainedEnabled == 1,
            "TAR receiver Time Constrained callback");
         LogicalTimeInterval lookahead = offset(publisherEpsilon, 5);
         publisher.enableTimeRegulation(lookahead);
         await(publisher, callbackModel,
            () -> publisherCallbacks.timeRegulationEnabled == 1,
            "TAR publisher Time Regulation callback");

         LogicalTime<?, ?> messageTime = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime<?, ?> publisherTarget = offset(publisherInitial, publisherEpsilon, 2);
         LogicalTime<?, ?> receiverTarget = offset(receiverInitial, receiverEpsilon, 6);
         byte[] payload = {0x54, 0x41, 0x52, 0x2d, 0x35};
         byte[] tag = {0x54, 0x41, 0x52, 0x2d, 0x35};
         MessageRetractionReturn retraction = publisher.sendInteraction(
            publisherClass, oneParameter(publisher, publisherParameter, payload),
            tag.clone(), messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "TAR queued timestamped interaction returned no valid retraction handle");
         pumpUnexpectedCallbacks(receiver, callbackModel, receiverCallbacks);
         JavaTckSupport.check(receiverCallbacks.interactions.isEmpty(),
            "TAR queued TSO was delivered before the receiver advance request");

         publisher.timeAdvanceRequest(publisherTarget);
         await(publisher, callbackModel,
            () -> publisherCallbacks.grants.size() == 1,
            "TAR regulator TAR(2) grant");
         JavaTckSupport.check(publisherTarget.equals(publisher.queryLogicalTime()),
            "TAR regulator did not reach its requested time");
         pumpUnexpectedCallbacks(receiver, callbackModel, receiverCallbacks);
         JavaTckSupport.check(receiverCallbacks.interactions.isEmpty(),
            "TAR queued TSO was delivered before the receiver TAR was submitted");

         receiverCallbacks.callbackOrder.clear();
         receiver.timeAdvanceRequest(receiverTarget);
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() == 1
               && receiverCallbacks.grants.size() == 1,
            "TAR queued TSO and grant callbacks");
         Interaction delivery = receiverCallbacks.interactions.get(0);
         JavaTckSupport.check(receiverClass.equals(delivery.interactionClass)
               && delivery.parameters.size() == 1
               && Arrays.equals(payload, delivery.parameters.get(receiverParameter))
               && Arrays.equals(tag, delivery.tag)
               && producer.equals(delivery.producer)
               && messageTime.equals(delivery.time)
               && delivery.sentOrder == OrderType.TIMESTAMP
               && delivery.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(delivery.retraction),
            "TAR delivered the wrong queued timestamped interaction or metadata");
         JavaTckSupport.check(receiverTarget.equals(receiverCallbacks.grants.get(0))
               && receiverTarget.equals(receiver.queryLogicalTime()),
            "TAR grant callback or queried logical time did not match the requested value");
         JavaTckSupport.check(receiverCallbacks.callbackOrder.equals(
               Arrays.asList("interaction", "grant")),
            "TAR delivered the queued TSO after its time-advance grant");

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

   /** TAR must finish its known below-target TSO cohort before granting. */
   static void runStrictCutoff(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-tar-strict-cutoff-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-tar-cutoff-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-tar-cutoff-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherClass =
            publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle receiverClass =
            receiver.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherClass, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverClass, parameterName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherParameter.equals(receiverParameter),
            "TAR cutoff adapter FOM did not resolve matching interaction handles");
         publisher.publishInteractionClass(publisherClass);
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "TAR cutoff members selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "TAR cutoff members began at different logical times");
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.timeConstrainedEnabled == 1,
            "TAR cutoff receiver constrained callback");
         publisher.enableTimeRegulation(publisherEpsilon);
         await(publisher, callbackModel,
            () -> publisherCallbacks.timeRegulationEnabled == 1,
            "TAR cutoff publisher regulation callback");

         LogicalTime<?, ?> five = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime<?, ?> six = offset(receiverInitial, receiverEpsilon, 6);
         LogicalTime<?, ?> eleven = offset(publisherInitial, publisherEpsilon, 11);
         LogicalTime<?, ?> twelve = offset(receiverInitial, receiverEpsilon, 12);
         byte[] payload = {0x54, 0x41, 0x52};
         byte[] tag = {'t', 'a', 'r'};
         ParameterHandleValueMap values =
            oneParameter(publisher, publisherParameter, payload);
         for (LogicalTime<?, ?> time : Arrays.asList(five, six, eleven)) {
            MessageRetractionReturn retraction = publisher.sendInteraction(
               publisherClass, values, tag.clone(), time);
            JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
                  && retraction.handle != null,
               "TAR cutoff send returned no valid retraction handle");
         }
         JavaTckSupport.check(receiverCallbacks.interactions.isEmpty(),
            "TAR cutoff delivered TSO before the receiver advanced");

         publisher.timeAdvanceRequest(six);
         await(publisher, callbackModel,
            () -> publisherCallbacks.grants.size() == 1,
            "TAR cutoff publisher first grant");
         receiver.timeAdvanceRequest(six);
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() >= 2
               && receiverCallbacks.grants.size() >= 1,
            "TAR cutoff receiver first grant");
         JavaTckSupport.check(receiverCallbacks.interactions.size() == 2
               && receiverCallbacks.grants.size() == 1
               && six.equals(receiverCallbacks.grants.get(0))
               && receiverCallbacks.callbackOrder.equals(
                  Arrays.asList("interaction", "interaction", "grant")),
            "TAR(6) did not finish its known TSO cohort before the grant");
         checkCutoffInteraction(receiverCallbacks.interactions.get(0), receiverClass,
            receiverParameter, producer, payload, tag, five);
         checkCutoffInteraction(receiverCallbacks.interactions.get(1), receiverClass,
            receiverParameter, producer, payload, tag, six);

         publisher.timeAdvanceRequest(twelve);
         await(publisher, callbackModel,
            () -> publisherCallbacks.grants.size() == 2,
            "TAR cutoff publisher final grant");
         receiver.timeAdvanceRequest(twelve);
         await(receiver, callbackModel,
            () -> receiverCallbacks.interactions.size() >= 3
               && receiverCallbacks.grants.size() >= 2,
            "TAR cutoff receiver second grant");
         JavaTckSupport.check(receiverCallbacks.interactions.size() == 3
               && receiverCallbacks.grants.size() == 2
               && twelve.equals(receiverCallbacks.grants.get(1))
               && receiverCallbacks.callbackOrder.equals(Arrays.asList(
                  "interaction", "interaction", "grant", "interaction", "grant")),
            "TAR(12) did not release only the later TSO before its grant");
         checkCutoffInteraction(receiverCallbacks.interactions.get(2), receiverClass,
            receiverParameter, producer, payload, tag, eleven);

         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void checkCutoffInteraction(Interaction delivery,
         InteractionClassHandle interaction, ParameterHandle parameter,
         FederateHandle producer, byte[] payload, byte[] tag, LogicalTime<?, ?> time) {
      JavaTckSupport.check(interaction.equals(delivery.interactionClass)
            && delivery.parameters.size() == 1
            && Arrays.equals(payload, delivery.parameters.get(parameter))
            && Arrays.equals(tag, delivery.tag)
            && producer.equals(delivery.producer)
            && time.equals(delivery.time)
            && delivery.sentOrder == OrderType.TIMESTAMP
            && delivery.receivedOrder == OrderType.TIMESTAMP,
         "TAR cutoff changed interaction callback content or metadata");
   }

   private static ParameterHandleValueMap oneParameter(RTIambassador ambassador,
         ParameterHandle parameter, byte[] payload) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload.clone());
      return values;
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
      private final hla.rti1516_2025.MessageRetractionHandle retraction;

      private Interaction(Object[] arguments) {
         interactionClass = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (hla.rti1516_2025.MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile int timeConstrainedEnabled;
      private volatile int timeRegulationEnabled;
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
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
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK TAR queued TSO callbacks";
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
