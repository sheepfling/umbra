package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
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

/** Verifies that an acknowledged pre-target TSO precedes a safe TAR grant. */
final class TarAcknowledgedTsoGaltReleaseTck {
   private TarAcknowledgedTsoGaltReleaseTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      String federation = "java-tck-tar-acknowledged-tso-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false;
      boolean publisherJoined = false, receiverJoined = false, created = false;
      try {
         publisher.connect(publisherEvents.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverEvents.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-tar-ack-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-tar-ack-receiver", federation);
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
            "Acknowledged-TSO TAR adapter FOM returned inconsistent handles");
         publisher.publishInteractionClass(publisherClass);
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "Acknowledged-TSO TAR members selected different time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "Acknowledged-TSO TAR members began at different times");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverEvents.constrainedEnabled == 1,
            "acknowledged-TSO receiver constrained callback");
         publisher.enableTimeRegulation(epsilon);
         await(publisher, callbackModel,
            () -> publisherEvents.regulationEnabled == 1,
            "acknowledged-TSO publisher regulation callback");

         LogicalTime<?, ?> messageTime = offset(publisherInitial, epsilon, 3);
         LogicalTime<?, ?> publisherTarget = offset(publisherInitial, epsilon, 5);
         LogicalTime<?, ?> receiverTarget = offset(receiverInitial, receiverEpsilon, 4);
         JavaTckSupport.check(compare(messageTime, receiverTarget) < 0
               && compare(receiverTarget, publisherTarget) < 0,
            "Acknowledged-TSO TAR fixture times are not ordered 3 < 4 < 5");
         byte[] value = {0x54, 0x41, 0x52};
         byte[] tag = {0x47, 0x41, 0x4c, 0x54};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, value.clone());
         MessageRetractionReturn retraction = publisher.sendInteraction(
            publisherClass, parameters, tag.clone(), messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null && receiverEvents.interactions.isEmpty(),
            "Acknowledged-TSO TAR did not retain the TSO before advance");

         // Advance the regulator beyond TAR(4) with one epsilon of lookahead.
         // Check only public callbacks and grants; do not prescribe a GALT formula.
         publisher.timeAdvanceRequest(publisherTarget);
         await(publisher, callbackModel,
            () -> publisherEvents.grants.size() == 1,
            "acknowledged-TSO regulator grant");
         JavaTckSupport.check(publisherTarget.equals(publisherEvents.grants.get(0)),
            "Acknowledged-TSO regulator granted at the wrong time");

         receiver.timeAdvanceRequest(receiverTarget);
         awaitBoth(publisher, receiver, callbackModel,
            () -> receiverEvents.interactions.size() >= 1
               && receiverEvents.grants.size() >= 1,
            "acknowledged-TSO callback-before-grant sequence");
         JavaTckSupport.check(receiverEvents.interactions.size() == 1
               && receiverEvents.grants.size() == 1
               && receiverEvents.receiveOrderOverloadCount == 0
               && receiverEvents.callbackOrder.equals(Arrays.asList("interaction", "grant")),
            "Acknowledged-TSO TAR did not deliver exactly once before grant");
         InteractionRecord delivered = receiverEvents.interactions.get(0);
         JavaTckSupport.check(receiverClass.equals(delivered.interaction)
               && delivered.values.size() == 1
               && Arrays.equals(value, delivered.values.get(receiverParameter))
               && Arrays.equals(tag, delivered.tag)
               && messageTime.equals(delivered.time)
               && receiverTarget.equals(receiverEvents.grants.get(0)),
            "Acknowledged-TSO TAR changed interaction data, time, or grant");

         receiver.disableTimeConstrained();
         publisher.disableTimeRegulation();
         receiver.unsubscribeInteractionClass(receiverClass);
         publisher.unpublishInteractionClass(publisherClass);
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

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compare(LogicalTime<?, ?> left, LogicalTime<?, ?> right) {
      return ((LogicalTime) left).compareTo((LogicalTime) right);
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador publisher, RTIambassador receiver,
         CallbackModel model, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            publisher.evokeCallback(0.0);
            receiver.evokeCallback(0.0);
         } else Thread.yield();
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

   private static final class InteractionRecord {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap values;
      private final byte[] tag;
      private final LogicalTime<?, ?> time;

      private InteractionRecord(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         values = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         time = (LogicalTime<?, ?>) arguments[6];
      }
   }

   private static final class Recorder {
      private volatile int constrainedEnabled;
      private volatile int regulationEnabled;
      private volatile int receiveOrderOverloadCount;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<InteractionRecord> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) constrainedEnabled++;
               else if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  callbackOrder.add("grant");
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  receiveOrderOverloadCount++;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  InteractionRecord event = new InteractionRecord(arguments);
                  callbackOrder.add("interaction");
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK acknowledged-TSO callbacks";
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
