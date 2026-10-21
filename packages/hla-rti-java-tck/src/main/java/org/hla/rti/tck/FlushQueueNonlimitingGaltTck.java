package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TimeQueryReturn;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks actual and optimistic FQG times when a verified GALT does not limit the request. */
final class FlushQueueNonlimitingGaltTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FlushQueueNonlimitingGaltTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder regulatorCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-fqg-nonlimiting-galt-" + UUID.randomUUID();
      boolean regulatorConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean regulatorJoined = false;
      boolean requesterJoined = false;
      try {
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         regulatorConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         regulator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         regulator.joinFederationExecution("java-tck-fqg-regulator", federation);
         regulatorJoined = true;
         requester.joinFederationExecution("java-tck-fqg-requester", federation);
         requesterJoined = true;

         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         JavaTckSupport.check(regulatorFactory.getName().equals(requesterFactory.getName()),
            "FQG nonlimiting-GALT members selected different time factories");
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         JavaTckSupport.check(regulatorInitial.equals(requesterInitial),
            "FQG nonlimiting-GALT members began at different times");
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorFactory.makeEpsilon();
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();

         regulator.enableTimeRegulation(regulatorEpsilon);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.regulationEnabled == 1,
            "FQG nonlimiting-GALT regulator enablement");
         LogicalTime<?, ?> regulatorTarget = offset(regulatorInitial, regulatorEpsilon, 12);
         regulator.timeAdvanceRequest(regulatorTarget);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.grants.size() == 1,
            "FQG nonlimiting-GALT regulator grant");
         JavaTckSupport.check(regulatorTarget.equals(regulatorCallbacks.grants.get(0)),
            "FQG nonlimiting-GALT regulator received the wrong grant time");

         requester.enableTimeConstrained();
         await(requester, callbackModel,
            () -> requesterCallbacks.constrainedEnabled == 1,
            "FQG nonlimiting-GALT constrained callback");
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         TimeQueryReturn galt = requester.queryGALT();
         JavaTckSupport.check(galt.timeIsValid && galt.time != null
               && compareTime(galt.time, requestTime) > 0,
            "FQG setup did not establish a defined GALT above the request");

         requester.flushQueueRequest(requestTime);
         await(requester, callbackModel,
            () -> requesterCallbacks.flushGrantTimes.size() == 1,
            "FQG nonlimiting-GALT grant");
         JavaTckSupport.check(requesterCallbacks.flushGrantTimes.size() == 1
               && requestTime.equals(requesterCallbacks.flushGrantTimes.get(0))
               && requestTime.equals(requesterCallbacks.optimisticTime)
               && requesterCallbacks.callbackOrder.equals(Arrays.asList("flush-grant"))
               && requestTime.equals(requester.queryLogicalTime()),
            "FQG actual, optimistic, or current logical time differed from the request");

         requester.disableTimeConstrained();
         regulator.disableTimeRegulation();
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         regulator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) resign(requester);
         if (regulatorJoined) resign(regulator);
         if (created) {
            try { regulator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (regulatorConnected) disconnect(regulator);
      }
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

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private volatile LogicalTime<?, ?> optimisticTime;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushGrantTimes = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  regulationEnabled++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  constrainedEnabled++;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  callbackOrder.add("grant");
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("flushQueueGrant".equals(name)
                     && arguments != null && arguments.length == 2) {
                  optimisticTime = (LogicalTime<?, ?>) arguments[1];
                  callbackOrder.add("flush-grant");
                  flushGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK FQG nonlimiting-GALT callbacks";
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
