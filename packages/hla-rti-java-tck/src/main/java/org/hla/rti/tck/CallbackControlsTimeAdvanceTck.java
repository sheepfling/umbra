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
import java.util.UUID;

/** P2 callback-control checks for time-advance grants. */
final class CallbackControlsTimeAdvanceTck {
   private CallbackControlsTimeAdvanceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder regulatorRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      String federation = "java-tck-callback-time-" + UUID.randomUUID();
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
         regulator.joinFederationExecution("java-tck-callback-time-regulator", federation);
         regulatorJoined = true;
         constrained.joinFederationExecution("java-tck-callback-time-constrained", federation);
         constrainedJoined = true;

         LogicalTimeFactory<?, ?> timeFactory = regulator.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(constrained.getTimeFactory().getName()),
            "time-management members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         regulator.enableTimeRegulation(epsilon);
         regulating = true;
         await(callbackModel, () -> regulatorRecorder.timeRegulationEnabled == 1, regulator);
         constrained.enableTimeConstrained();
         timeConstrained = true;
         await(callbackModel, () -> constrainedRecorder.timeConstrainedEnabled == 1, constrained);

         LogicalTime regulatorTarget = add(regulator.queryLogicalTime(), intervalOffset(epsilon, 4));
         LogicalTime constrainedTarget = add(
            constrained.queryLogicalTime(), intervalOffset(epsilon, 4));
         JavaTckSupport.check(regulatorTarget.equals(constrainedTarget),
            "time-managed federates did not derive the same advance target");

         constrained.disableCallbacks();
         regulator.timeAdvanceRequest(regulatorTarget);
         constrained.timeAdvanceRequest(constrainedTarget);
         await(callbackModel, () -> regulatorRecorder.timeAdvanceGrant != null, regulator);

         serviceCallbackWork(constrained, callbackModel, objectClassName);
         JavaTckSupport.check(constrainedRecorder.timeAdvanceGrant == null,
            "disabled callbacks exposed a time-advance grant");

         constrained.enableCallbacks();
         serviceCallbackWork(constrained, callbackModel, objectClassName);
         await(callbackModel, () -> constrainedRecorder.timeAdvanceGrant != null, constrained);
         JavaTckSupport.check(regulatorTarget.equals(regulatorRecorder.timeAdvanceGrant)
               && constrainedTarget.equals(constrainedRecorder.timeAdvanceGrant),
            "re-enabled time-advance callbacks returned the wrong target time");
      } finally {
         if (timeConstrained && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && regulatorJoined) {
            try { regulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (constrainedJoined) resign(constrained);
         if (regulatorJoined) resign(regulator);
         if (created) {
            try { regulator.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (constrainedConnected) disconnect(constrained);
         if (regulatorConnected) disconnect(regulator);
      }
   }

   private static void serviceCallbackWork(RTIambassador ambassador,
         CallbackModel callbackModel, String objectClassName) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 1.0);
      } else {
         ambassador.getObjectClassHandle(objectClassName);
      }
   }

   private static LogicalTime add(LogicalTime start, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) start.add(interval);
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static void await(CallbackModel callbackModel, Condition condition,
         RTIambassador ambassador) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         if (condition.isMet()) return;
         Thread.yield();
      }
      throw new AssertionError("time-advance callback did not arrive before deadline");
   }

   @FunctionalInterface
   private interface Condition {
      boolean isMet() throws Exception;
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
      } catch (Exception ignored) {
      }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
   }

   private static final class Recorder {
      private int timeRegulationEnabled;
      private int timeConstrainedEnabled;
      private LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled++;
               if ("timeConstrainedEnabled".equals(name)) timeConstrainedEnabled++;
               if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK callback-controlled time callbacks";
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
