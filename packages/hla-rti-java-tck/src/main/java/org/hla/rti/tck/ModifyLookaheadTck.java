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

/** Focused standard-API test for lookahead changes and their grant boundary. */
final class ModifyLookaheadTck {
   private ModifyLookaheadTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder regulatorRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      String federation = "java-tck-modify-lookahead-" + UUID.randomUUID();
      boolean regulatorConnected = false;
      boolean constrainedConnected = false;
      boolean regulatorJoined = false;
      boolean constrainedJoined = false;
      boolean created = false;
      boolean regulating = false;
      boolean constrainedRole = false;
      try {
         regulator.connect(regulatorRecorder.proxy(), callbackModel);
         regulatorConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;
         regulator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         regulator.joinFederationExecution("java-tck-lookahead-regulator", federation);
         regulatorJoined = true;
         constrained.joinFederationExecution("java-tck-lookahead-constrained", federation);
         constrainedJoined = true;

         LogicalTimeFactory<?, ?> timeFactory = regulator.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(constrained.getTimeFactory().getName()),
            "time-management members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         JavaTckSupport.expectFailure(() -> regulator.modifyLookahead(epsilon),
            "TimeRegulationIsNotEnabled", "Modify Lookahead before Time Regulation");
         LogicalTimeInterval initialLookahead = intervalOffset(epsilon, 3);
         LogicalTimeInterval increasedLookahead = intervalOffset(epsilon, 6);
         regulator.enableTimeRegulation(initialLookahead);
         regulating = true;
         JavaTckSupport.drain(regulator);
         constrained.enableTimeConstrained();
         constrainedRole = true;
         JavaTckSupport.drain(constrained);
         JavaTckSupport.check(regulatorRecorder.timeRegulationEnabled == 1
               && constrainedRecorder.timeConstrainedEnabled == 1,
            "time-management roles did not complete through their standard callbacks");
         JavaTckSupport.expectFailure(() -> regulator.enableTimeRegulation(initialLookahead),
            "TimeRegulationAlreadyEnabled", "enabling time regulation twice");
         JavaTckSupport.expectFailure(() -> constrained.enableTimeConstrained(),
            "TimeConstrainedAlreadyEnabled", "enabling time constrained twice");

         JavaTckSupport.check(initialLookahead.equals(regulator.queryLookahead()),
            "Query Lookahead did not return the enabled lookahead");
         regulator.modifyLookahead(increasedLookahead);
         JavaTckSupport.check(increasedLookahead.equals(regulator.queryLookahead()),
            "Query Lookahead did not expose an increased lookahead immediately");

         regulator.modifyLookahead(epsilon);
         JavaTckSupport.check(increasedLookahead.equals(regulator.queryLookahead()),
            "a lower requested lookahead was applied before a grant");

         LogicalTime initialRegulatorTime = regulator.queryLogicalTime();
         LogicalTime initialConstrainedTime = constrained.queryLogicalTime();
         LogicalTime target = add(initialRegulatorTime, intervalOffset(epsilon, 3));
         LogicalTime constrainedTarget = add(
            initialConstrainedTime, intervalOffset(epsilon, 3));
         JavaTckSupport.check(target.equals(constrainedTarget),
            "federates did not derive a common advance target");
         regulator.timeAdvanceRequest(target);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.expectFailure(() -> regulator.timeAdvanceRequest(target),
               "InTimeAdvancingState", "second time advance request while the first is pending");
            JavaTckSupport.expectFailure(() -> regulator.modifyLookahead(increasedLookahead),
               "InTimeAdvancingState", "Modify Lookahead during a pending TAR");
            JavaTckSupport.check(increasedLookahead.equals(regulator.queryLookahead()),
               "a rejected pending-TAR Modify Lookahead changed the active interval");
            JavaTckSupport.check(initialRegulatorTime.equals(regulator.queryLogicalTime()),
               "regulator logical time changed before Time Advance Grant");
            JavaTckSupport.check(initialConstrainedTime.equals(constrained.queryLogicalTime()),
               "constrained logical time changed before Time Advance Grant");
         }
         constrained.timeAdvanceRequest(constrainedTarget);
         JavaTckSupport.drain(regulator);
         JavaTckSupport.drain(constrained);
         JavaTckSupport.check(regulatorRecorder.timeAdvanceGrants == 1
               && constrainedRecorder.timeAdvanceGrants == 1,
            "the lookahead boundary TAR did not complete for both federates");
         JavaTckSupport.check(target.equals(regulator.queryLogicalTime()),
            "the regulator logical time did not advance to the requested target");
         JavaTckSupport.check(constrainedTarget.equals(constrained.queryLogicalTime()),
            "the constrained logical time did not advance to the requested target");
         JavaTckSupport.check(intervalOffset(epsilon, 3).equals(regulator.queryLookahead()),
            "the pending lookahead did not decrease by the first grant's time advance");

         LogicalTime finalTarget = add(initialRegulatorTime, intervalOffset(epsilon, 6));
         LogicalTime constrainedFinalTarget = add(
            initialConstrainedTime, intervalOffset(epsilon, 6));
         regulator.timeAdvanceRequest(finalTarget);
         constrained.timeAdvanceRequest(constrainedFinalTarget);
         JavaTckSupport.drain(regulator);
         JavaTckSupport.drain(constrained);
         JavaTckSupport.check(regulatorRecorder.timeAdvanceGrants == 2
               && constrainedRecorder.timeAdvanceGrants == 2,
            "the second gradual-decrease TAR did not complete for both federates");
         JavaTckSupport.check(finalTarget.equals(regulator.queryLogicalTime())
               && constrainedFinalTarget.equals(constrained.queryLogicalTime()),
            "the second grant did not reach the requested logical times");
         JavaTckSupport.check(epsilon.equals(regulator.queryLookahead()),
            "the pending lookahead did not reach its requested value after the second grant");
         LogicalTime earlierTarget = add(initialRegulatorTime, intervalOffset(epsilon, 3));
         JavaTckSupport.expectFailure(() -> regulator.timeAdvanceRequest(earlierTarget),
            "LogicalTimeAlreadyPassed", "time advance request behind granted logical time");

         constrained.disableTimeConstrained();
         constrainedRole = false;
         JavaTckSupport.expectFailure(() -> constrained.disableTimeConstrained(),
            "TimeConstrainedIsNotEnabled", "disabling time constrained twice");
         regulator.disableTimeRegulation();
         regulating = false;
         JavaTckSupport.expectFailure(() -> regulator.disableTimeRegulation(),
            "TimeRegulationIsNotEnabled", "disabling time regulation twice");
      } finally {
         if (constrainedRole && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && regulatorJoined) {
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

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static final class Recorder {
      private int timeRegulationEnabled;
      private int timeConstrainedEnabled;
      private int timeAdvanceGrants;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled++;
               if ("timeConstrainedEnabled".equals(name)) timeConstrainedEnabled++;
               if ("timeAdvanceGrant".equals(name)) timeAdvanceGrants++;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK lookahead callbacks";
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
