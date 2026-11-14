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
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** A non-regulating member's TAR must not constrain a constrained peer. */
final class NonRegulatingFederatePeerAdvanceTck {
   private NonRegulatingFederatePeerAdvanceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador nonRegulator = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder nonRegulatorRecorder = new Recorder("non-regulating peer");
      Recorder regulatorRecorder = new Recorder("regulator");
      Recorder constrainedRecorder = new Recorder("time-constrained peer");
      RTIambassador[] participants = {nonRegulator, regulator, constrained};
      String federation = "java-tck-non-regulating-peer-advance-" + UUID.randomUUID();
      boolean nonRegulatorConnected = false;
      boolean regulatorConnected = false;
      boolean constrainedConnected = false;
      boolean created = false;
      boolean nonRegulatorJoined = false;
      boolean regulatorJoined = false;
      boolean constrainedJoined = false;
      boolean regulating = false;
      boolean timeConstrained = false;
      try {
         nonRegulator.connect(nonRegulatorRecorder.proxy(), callbackModel);
         nonRegulatorConnected = true;
         regulator.connect(regulatorRecorder.proxy(), callbackModel);
         regulatorConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;

         regulator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         regulator.joinFederationExecution("java-tck-advancing-regulator", federation);
         regulatorJoined = true;
         nonRegulator.joinFederationExecution("java-tck-non-regulating-peer", federation);
         nonRegulatorJoined = true;
         constrained.joinFederationExecution("java-tck-time-constrained-peer", federation);
         constrainedJoined = true;

         LogicalTimeFactory<?, ?> nonRegulatorFactory = nonRegulator.getTimeFactory();
         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         LogicalTimeFactory<?, ?> constrainedFactory = constrained.getTimeFactory();
         JavaTckSupport.check(nonRegulatorFactory.getName().equals(regulatorFactory.getName())
               && regulatorFactory.getName().equals(constrainedFactory.getName()),
            "non-regulating peer advancement selected inconsistent logical-time factories");

         LogicalTime<?, ?> initial = nonRegulatorFactory.makeInitial();
         LogicalTime<?, ?> regulatorInitial = regulatorFactory.makeInitial();
         LogicalTime<?, ?> constrainedInitial = constrainedFactory.makeInitial();
         JavaTckSupport.check(initial.equals(regulatorInitial)
               && initial.equals(constrainedInitial)
               && initial.equals(nonRegulator.queryLogicalTime())
               && regulatorInitial.equals(regulator.queryLogicalTime())
               && constrainedInitial.equals(constrained.queryLogicalTime()),
            "joined federates did not start at a shared logical-time initial value");

         LogicalTimeInterval epsilon = (LogicalTimeInterval) nonRegulatorFactory.makeEpsilon();
         LogicalTime<?, ?> nonRegulatorTarget = offset(initial, epsilon, 1);
         nonRegulator.timeAdvanceRequest(nonRegulatorTarget);
         await(callbackModel, participants,
            () -> nonRegulatorRecorder.timeAdvanceGrantTimes.size() == 1,
            "non-regulating peer's time-1 grant");
         JavaTckSupport.check(nonRegulatorTarget.equals(
               nonRegulatorRecorder.timeAdvanceGrantTimes.get(0))
               && nonRegulatorTarget.equals(nonRegulator.queryLogicalTime()),
            "non-regulating peer received a grant at the wrong time");

         LogicalTime<?, ?> regulatorTarget = offset(initial, epsilon, 10);
         regulator.timeAdvanceRequest(regulatorTarget);
         await(callbackModel, participants,
            () -> regulatorRecorder.timeAdvanceGrantTimes.size() == 1,
            "regulator's time-10 grant before enabling regulation");
         JavaTckSupport.check(regulatorTarget.equals(
               regulatorRecorder.timeAdvanceGrantTimes.get(0))
               && regulatorTarget.equals(regulator.queryLogicalTime()),
            "regulator received a grant at the wrong time before enabling regulation");

         regulator.enableTimeRegulation(epsilon);
         regulating = true;
         await(callbackModel, participants,
            () -> regulatorRecorder.timeRegulationEnabledTimes.size() == 1,
            "regulator's time-regulation callback");
         JavaTckSupport.check(regulatorTarget.equals(
               regulatorRecorder.timeRegulationEnabledTimes.get(0)),
            "regulator enabled at a time other than its completed grant");

         constrained.enableTimeConstrained();
         timeConstrained = true;
         await(callbackModel, participants,
            () -> constrainedRecorder.timeConstrainedEnabledTimes.size() == 1,
            "time-constrained peer's enablement callback");
         JavaTckSupport.check(constrainedInitial.equals(
               constrainedRecorder.timeConstrainedEnabledTimes.get(0)),
            "time-constrained peer did not enable at its initial time");

         LogicalTime<?, ?> constrainedTarget = offset(constrainedInitial, epsilon, 10);
         constrained.timeAdvanceRequest(constrainedTarget);
         await(callbackModel, participants,
            () -> constrainedRecorder.timeAdvanceGrantTimes.size() == 1,
            "time-constrained peer's time-10 grant");

         JavaTckSupport.check(constrainedTarget.equals(
               constrainedRecorder.timeAdvanceGrantTimes.get(0))
               && constrainedTarget.equals(constrained.queryLogicalTime()),
            "time-constrained peer did not advance to time 10 under the regulator's frontier");
         JavaTckSupport.check(nonRegulatorRecorder.timeAdvanceGrantTimes.size() == 1
               && nonRegulatorTarget.equals(nonRegulator.queryLogicalTime()),
            "non-regulating peer's completed time-1 grant constrained peer advancement");
         JavaTckSupport.check(regulatorRecorder.timeAdvanceGrantTimes.size() == 1
               && regulatorTarget.equals(regulator.queryLogicalTime()),
            "constrained peer advancement changed the regulator's completed grant");

         constrained.disableTimeConstrained();
         timeConstrained = false;
         regulator.disableTimeRegulation();
         regulating = false;
         constrained.resignFederationExecution(ResignAction.NO_ACTION);
         constrainedJoined = false;
         nonRegulator.resignFederationExecution(ResignAction.NO_ACTION);
         nonRegulatorJoined = false;
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
         if (nonRegulatorJoined) {
            try { nonRegulator.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (nonRegulatorConnected) {
            try { nonRegulator.disconnect(); }
            catch (Exception ignored) { }
         }
         if (regulatorConnected) {
            try { regulator.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime<?, ?> result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime<?, ?>) ((LogicalTime) result).add(interval);
      }
      return result;
   }

   private static void await(CallbackModel callbackModel, RTIambassador[] participants,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         for (RTIambassador participant : participants) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               participant.evokeMultipleCallbacks(0.0, 0.01);
            } else {
               participant.queryLogicalTime();
            }
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class Recorder {
      private final String description;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeRegulationEnabledTimes = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeConstrainedEnabledTimes = new ArrayList<>();

      private Recorder(String description) {
         this.description = description;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeAdvanceGrant".equals(name)) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabledTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabledTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return description + " callback recorder";
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
