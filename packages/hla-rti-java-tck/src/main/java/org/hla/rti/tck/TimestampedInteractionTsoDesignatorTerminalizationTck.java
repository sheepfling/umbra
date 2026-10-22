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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks retraction terminalization after each standard advance designator. */
final class TimestampedInteractionTsoDesignatorTerminalizationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedInteractionTsoDesignatorTerminalizationTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-interaction-tso-terminalization-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean created = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-tso-terminalization-producer", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-tso-terminalization-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherClass =
            publisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherClass, parameterName);
         InteractionClassHandle receiverClass =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverClass, parameterName);
         publisher.publishInteractionClass(publisherClass);
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "timestamped interaction members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime lookaheadTarget = offset(initial, epsilon, 5);
         LogicalTimeInterval lookahead = (LogicalTimeInterval) lookaheadTarget.distance(initial);

         receiver.enableTimeConstrained();
         await(receiver, callbackModel, () -> receiverRecorder.timeConstrainedEnabled,
            "receiver Time Constrained callback");
         publisher.enableTimeRegulation(lookahead);
         await(publisher, callbackModel, () -> publisherRecorder.timeRegulationEnabled,
            "publisher Time Regulation callback");
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabled
               && publisherRecorder.timeRegulationEnabled,
            "time-managed role callbacks were not delivered through the API");

         AdvanceService[] services = new AdvanceService[] {
            (ambassador, target) -> ambassador.timeAdvanceRequest(target),
            (ambassador, target) -> ambassador.timeAdvanceRequestAvailable(target),
            (ambassador, target) -> ambassador.nextMessageRequest(target),
            (ambassador, target) -> ambassador.nextMessageRequestAvailable(target)
         };
         String[] names = {"TAR", "TARA", "NMR", "NMRA"};
         for (int i = 0; i < services.length; ++i) {
            int timestampSteps = i + 6;
            MessageRetractionReturn sent = send(publisher, publisherClass, publisherParameter,
               offset(initial, epsilon, timestampSteps));
            checkRetraction(sent, names[i]);
            LogicalTime target = offset(initial, epsilon, timestampSteps - 5);
            int previousGrantCount = publisherRecorder.timeAdvanceGrantTimes.size();
            services[i].request(publisher, target);
            await(publisher, callbackModel,
               () -> publisherRecorder.timeAdvanceGrantTimes.size() == previousGrantCount + 1,
               names[i] + " Time Advance Grant");
            JavaTckSupport.check(publisherRecorder.timeAdvanceGrantTimes.size()
                  == previousGrantCount + 1
                  && target.equals(publisherRecorder.timeAdvanceGrantTimes.get(previousGrantCount))
                  && target.equals(publisher.queryLogicalTime()),
               names[i] + " did not grant and report its requested logical time");
            expectException(() -> publisher.retract(sent.handle),
               "MessageCanNoLongerBeRetracted", names[i] + " terminal retraction");
            JavaTckSupport.check(receiverRecorder.interactionCount == 0,
               names[i] + " delivered a timestamped interaction to an idle constrained receiver");
         }

         MessageRetractionReturn flushSent = send(publisher, publisherClass, publisherParameter,
            offset(initial, epsilon, 10));
         checkRetraction(flushSent, "Flush Queue Request");
         LogicalTime flushBoundary = offset(initial, epsilon, 5);
         publisher.flushQueueRequest(flushBoundary);
         await(publisher, callbackModel,
            () -> publisherRecorder.flushQueueGrantTimes.size() == 1,
            "Flush Queue Grant");
         JavaTckSupport.check(publisherRecorder.flushQueueGrantTimes.size() == 1
               && publisherRecorder.flushQueueGrantTime != null
               && publisherRecorder.flushQueueOptimisticTime != null
               && compareTime(publisherRecorder.flushQueueGrantTime, initial) >= 0
               && compareTime(publisherRecorder.flushQueueGrantTime, flushBoundary) <= 0
               && compareTime(publisherRecorder.flushQueueOptimisticTime,
                  publisherRecorder.flushQueueGrantTime) >= 0
               && publisherRecorder.flushQueueGrantTime.equals(publisher.queryLogicalTime()),
            "Flush Queue Request returned an invalid grant or query time");
         expectException(() -> publisher.retract(flushSent.handle),
            "MessageCanNoLongerBeRetracted", "Flush Queue Request terminal retraction");
         JavaTckSupport.check(receiverRecorder.interactionCount == 0,
            "Flush Queue Request delivered to an idle constrained receiver");
      } finally {
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static MessageRetractionReturn send(RTIambassador publisher,
         InteractionClassHandle interactionClass, ParameterHandle parameter, LogicalTime time)
         throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, new byte[] {0x54, 0x53, 0x4f});
      return publisher.sendInteraction(interactionClass, values, new byte[] {0x54, 0x53, 0x4f}, time);
   }

   private static void checkRetraction(MessageRetractionReturn result, String description) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null, description + " send returned no valid retraction handle");
   }

   private static void expectException(JavaTckSupport.CheckedOperation operation,
         String expected, String description) throws Exception {
      JavaTckSupport.expectFailure(operation, expected, description);
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compareTime(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   @FunctionalInterface
   private interface AdvanceService {
      void request(RTIambassador ambassador, LogicalTime<?, ?> target) throws Exception;
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private volatile int interactionCount;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes =
         new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes =
         new CopyOnWriteArrayList<>();
      private volatile LogicalTime<?, ?> flushQueueGrantTime;
      private volatile LogicalTime<?, ?> flushQueueOptimisticTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("receiveInteraction".equals(name)) {
                  interactionCount++;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("flushQueueGrant".equals(name)
                     && arguments != null && arguments.length == 2) {
                  flushQueueGrantTime = (LogicalTime<?, ?>) arguments[0];
                  flushQueueOptimisticTime = (LogicalTime<?, ?>) arguments[1];
                  flushQueueGrantTimes.add(flushQueueGrantTime);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK TSO terminalization callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
