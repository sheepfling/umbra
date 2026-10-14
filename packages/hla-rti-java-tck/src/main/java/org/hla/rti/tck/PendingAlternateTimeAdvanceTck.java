package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
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

/** Focused standard-API check for competing advances while TAR is pending. */
final class PendingAlternateTimeAdvanceTck {
   private PendingAlternateTimeAdvanceTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder regulatorRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      String federation = "java-tck-pending-alternate-advance-" + UUID.randomUUID();
      boolean regulatorConnected = false;
      boolean constrainedConnected = false;
      boolean regulatorJoined = false;
      boolean constrainedJoined = false;
      boolean created = false;
      boolean regulationEnabled = false;
      boolean constrainedEnabled = false;
      try {
         regulator.connect(regulatorRecorder.proxy(), callbackModel);
         regulatorConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;
         regulator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         regulator.joinFederationExecution("java-tck-pending-advance-regulator", federation);
         regulatorJoined = true;
         constrained.joinFederationExecution("java-tck-pending-advance-constrained", federation);
         constrainedJoined = true;

         LogicalTimeFactory<?, ?> timeFactory = regulator.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(constrained.getTimeFactory().getName()),
            "time-management members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         regulator.enableTimeRegulation(epsilon);
         regulationEnabled = true;
         await(callbackModel, regulator, () -> regulatorRecorder.timeRegulationEnabled == 1,
            "timeRegulationEnabled callback");
         constrained.enableTimeConstrained();
         constrainedEnabled = true;
         await(callbackModel, constrained,
            () -> constrainedRecorder.timeConstrainedEnabled == 1,
            "timeConstrainedEnabled callback");
         JavaTckSupport.check(regulatorRecorder.timeRegulationEnabled == 1
               && constrainedRecorder.timeConstrainedEnabled == 1,
            "time-management roles did not complete through their standard callbacks");

         LogicalTime constrainedTarget = add(constrained.queryLogicalTime(), epsilon);
         LogicalTime regulatorTarget = add(regulator.queryLogicalTime(), epsilon);
         constrained.timeAdvanceRequest(constrainedTarget);
         expectInTimeAdvancingState(
            () -> constrained.timeAdvanceRequestAvailable(constrainedTarget),
            "TARA while TAR is pending");
         expectInTimeAdvancingState(
            () -> constrained.nextMessageRequestAvailable(constrainedTarget),
            "NMRA while TAR is pending");
         expectInTimeAdvancingState(
            () -> constrained.flushQueueRequest(constrainedTarget),
            "Flush Queue Request while TAR is pending");
         expectInTimeAdvancingState(
            () -> constrained.timeAdvanceRequest(constrainedTarget),
            "second TAR while TAR is pending");
         JavaTckSupport.check(constrainedRecorder.timeAdvanceGrantTimes.isEmpty(),
            "rejected competing requests completed the original pending TAR");

         regulator.timeAdvanceRequest(regulatorTarget);
         await(callbackModel, regulator,
            () -> !regulatorRecorder.timeAdvanceGrantTimes.isEmpty(),
            "regulator timeAdvanceGrant callback");
         await(callbackModel, constrained,
            () -> !constrainedRecorder.timeAdvanceGrantTimes.isEmpty(),
            "constrained timeAdvanceGrant callback");
         JavaTckSupport.check(regulatorRecorder.timeAdvanceGrantTimes.size() == 1
               && constrainedRecorder.timeAdvanceGrantTimes.size() == 1,
            "the original pending TAR did not complete with exactly one grant per federate");
         JavaTckSupport.check(regulatorTarget.equals(
                  regulatorRecorder.timeAdvanceGrantTimes.get(0))
               && constrainedTarget.equals(constrainedRecorder.timeAdvanceGrantTimes.get(0)),
            "the pending TAR sequence reported an incorrect grant time");

         constrained.disableTimeConstrained();
         constrainedEnabled = false;
         regulator.disableTimeRegulation();
         regulationEnabled = false;
      } finally {
         if (constrainedEnabled && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulationEnabled && regulatorJoined) {
            try { regulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (constrainedJoined) {
            try { constrained.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (regulatorJoined) {
            try { regulator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { regulator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (constrainedConnected) {
            try { constrained.disconnect(); } catch (Exception ignored) { }
         }
         if (regulatorConnected) {
            try { regulator.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime add(LogicalTime start, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) start.add(interval);
   }

   private static void await(CallbackModel model, RTIambassador ambassador,
         java.util.function.BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void expectInTimeAdvancingState(
         JavaTckSupport.CheckedOperation operation, String description) throws Exception {
      JavaTckSupport.expectFailure(operation, "InTimeAdvancingState", description);
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes =
         new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK time callbacks";
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
