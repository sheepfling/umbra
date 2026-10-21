package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Checks the Time Constrained Enabled argument for an already regulating federate. */
final class TimeConstrainedRegulatingCurrentTimeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeConstrainedRegulatingCurrentTimeTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder regulatorCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-time-constrained-regulating-current-"
         + UUID.randomUUID();
      boolean regulatorConnected = false;
      boolean requesterConnected = false;
      boolean federationCreated = false;
      boolean regulatorJoined = false;
      boolean requesterJoined = false;
      try {
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         regulatorConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         regulator.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         regulator.joinFederationExecution("java-tck-current-regulator", federation);
         regulatorJoined = true;
         requester.joinFederationExecution("java-tck-current-requester", federation);
         requesterJoined = true;

         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         JavaTckSupport.check(regulatorFactory.getName().equals(requesterFactory.getName()),
            "Time Constrained current-time members selected different logical-time factories");
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         JavaTckSupport.check(regulatorInitial.equals(requesterInitial),
            "Time Constrained current-time members began at different logical times");
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorFactory.makeEpsilon();
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();

         regulator.enableTimeRegulation(regulatorEpsilon);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.timeRegulationEnabled == 1,
            "Time Constrained current-time regulator enable callback");
         LogicalTime<?, ?> regulatorFive = offset(regulatorInitial, regulatorEpsilon, 5);
         LogicalTime<?, ?> requesterFive = offset(requesterInitial, requesterEpsilon, 5);
         regulator.timeAdvanceRequest(regulatorFive);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.grants == 1,
            "Time Constrained current-time regulator grant");
         JavaTckSupport.check(regulatorFive.equals(regulatorCallbacks.grantTime),
            "Time Constrained current-time regulator granted at the wrong time");

         requester.timeAdvanceRequest(requesterFive);
         await(requester, callbackModel,
            () -> requesterCallbacks.grants == 1,
            "Time Constrained current-time requester grant");
         JavaTckSupport.check(requesterFive.equals(requesterCallbacks.grantTime),
            "Time Constrained current-time requester granted at the wrong time");
         requester.enableTimeRegulation(requesterEpsilon);
         await(requester, callbackModel,
            () -> requesterCallbacks.timeRegulationEnabled == 1,
            "Time Constrained current-time requester regulation callback");

         LogicalTime<?, ?> beforeEnable = requester.queryLogicalTime();
         JavaTckSupport.check(requesterFive.equals(beforeEnable),
            "Time Constrained current-time query changed after regulation enablement");
         requester.enableTimeConstrained();
         await(requester, callbackModel,
            () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "Time Constrained current-time enable callback");
         LogicalTime<?, ?> afterEnable = requester.queryLogicalTime();
         JavaTckSupport.check(beforeEnable.equals(afterEnable)
               && afterEnable.equals(requesterCallbacks.constrainedTime),
            "Time Constrained Enabled did not report the regulating federate's current logical time");

         requester.disableTimeConstrained();
         requester.disableTimeRegulation();
         regulator.disableTimeRegulation();
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         regulator.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (requesterJoined) resign(requester);
         if (regulatorJoined) resign(regulator);
         if (federationCreated) {
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
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private volatile int grants;
      private volatile LogicalTime<?, ?> grantTime;
      private volatile LogicalTime<?, ?> constrainedTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled++;
               } else if ("timeConstrainedEnabled".equals(name) && arguments != null
                     && arguments.length == 1) {
                  constrainedTime = (LogicalTime<?, ?>) arguments[0];
                  timeConstrainedEnabled++;
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grantTime = (LogicalTime<?, ?>) arguments[0];
                  grants++;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK regulating current-time callbacks";
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
