package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** Callback gating for time-regulating and time-constrained role enablement. */
final class CallbackControlsTimeRoleEnablementTck {
   private CallbackControlsTimeRoleEnablementTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder regulatorRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      String federation = "java-tck-callback-time-role-" + UUID.randomUUID();
      boolean regulatorConnected = false;
      boolean constrainedConnected = false;
      boolean created = false;
      boolean regulatorJoined = false;
      boolean constrainedJoined = false;
      boolean regulating = false;
      boolean timeConstrained = false;
      try {
         regulator.connect(regulatorRecorder.proxy(), callbackModel);
         regulatorConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;
         regulator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         regulator.joinFederationExecution("java-tck-time-regulator", federation);
         regulatorJoined = true;
         constrained.joinFederationExecution("java-tck-time-constrained", federation);
         constrainedJoined = true;

         JavaTckSupport.check(regulator.getTimeFactory().getName().equals(
               constrained.getTimeFactory().getName()),
            "time-role federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) regulator.getTimeFactory()
            .makeEpsilon();
         regulatorRecorder.clear();
         constrainedRecorder.clear();
         regulator.disableCallbacks();
         constrained.disableCallbacks();
         regulator.enableTimeRegulation(epsilon);
         regulating = true;
         constrained.enableTimeConstrained();
         timeConstrained = true;

         service(regulator, callbackModel, objectClassName);
         service(constrained, callbackModel, objectClassName);
         JavaTckSupport.check(regulatorRecorder.timeRegulationTimes.isEmpty()
               && constrainedRecorder.timeConstrainedTimes.isEmpty(),
            "disabled callbacks exposed a time-role enablement callback");

         regulator.enableCallbacks();
         constrained.enableCallbacks();
         await(regulator, callbackModel, objectClassName,
            () -> regulatorRecorder.timeRegulationTimes.size() == 1,
            "timeRegulationEnabled callback did not arrive after enabling callbacks");
         await(constrained, callbackModel, objectClassName,
            () -> constrainedRecorder.timeConstrainedTimes.size() == 1,
            "timeConstrainedEnabled callback did not arrive after enabling callbacks");
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> constrainedInitial = constrained.queryLogicalTime();
         JavaTckSupport.check(regulatorRecorder.timeRegulationTimes.size() == 1
               && regulatorInitial.equals(regulatorRecorder.timeRegulationTimes.get(0)),
            "timeRegulationEnabled returned the wrong initial logical time");
         JavaTckSupport.check(constrainedRecorder.timeConstrainedTimes.size() == 1
               && constrainedInitial.equals(constrainedRecorder.timeConstrainedTimes.get(0)),
            "timeConstrainedEnabled returned the wrong initial logical time");

         constrained.disableTimeConstrained();
         timeConstrained = false;
         regulator.disableTimeRegulation();
         regulating = false;
         constrained.resignFederationExecution(ResignAction.NO_ACTION);
         constrainedJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         regulator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (timeConstrained && constrainedJoined) {
            try { constrained.disableTimeConstrained(); }
            catch (Exception ignored) { }
         }
         if (regulating && regulatorJoined) {
            try { regulator.disableTimeRegulation(); }
            catch (Exception ignored) { }
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
            try { constrained.disconnect(); }
            catch (Exception ignored) { }
         }
         if (regulatorConnected) {
            try { regulator.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(ambassador, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 1.0);
      else ambassador.getObjectClassHandle(objectClassName);
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class Recorder {
      private final List<LogicalTime<?, ?>> timeRegulationTimes = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeConstrainedTimes = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK time-role callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clear() {
         timeRegulationTimes.clear();
         timeConstrainedTimes.clear();
      }
   }
}
