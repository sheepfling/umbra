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

         JavaTckSupport.check(initialLookahead.equals(regulator.queryLookahead()),
            "Query Lookahead did not return the enabled lookahead");
         regulator.modifyLookahead(increasedLookahead);
         JavaTckSupport.check(increasedLookahead.equals(regulator.queryLookahead()),
            "Query Lookahead did not expose an increased lookahead immediately");

         LogicalTimeInterval zero = (LogicalTimeInterval) timeFactory.makeZero();
         JavaTckSupport.expectFailure(() -> regulator.modifyLookahead(zero),
            "InvalidLookahead", "zero Modify Lookahead");
         JavaTckSupport.check(increasedLookahead.equals(regulator.queryLookahead()),
            "a rejected Modify Lookahead changed the active interval");

         regulator.modifyLookahead(epsilon);
         JavaTckSupport.check(increasedLookahead.equals(regulator.queryLookahead()),
            "a lower requested lookahead was applied before a grant");

         LogicalTime target = add(regulator.queryLogicalTime(), intervalOffset(epsilon, 12));
         LogicalTime constrainedTarget = add(
            constrained.queryLogicalTime(), intervalOffset(epsilon, 12));
         JavaTckSupport.check(target.equals(constrainedTarget),
            "federates did not derive a common advance target");
         regulator.timeAdvanceRequest(target);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.expectFailure(() -> regulator.modifyLookahead(increasedLookahead),
               "InTimeAdvancingState", "Modify Lookahead during a pending TAR");
            JavaTckSupport.check(increasedLookahead.equals(regulator.queryLookahead()),
               "a rejected pending-TAR Modify Lookahead changed the active interval");
         }
         constrained.timeAdvanceRequest(constrainedTarget);
         JavaTckSupport.drain(regulator);
         JavaTckSupport.drain(constrained);
         JavaTckSupport.check(regulatorRecorder.timeAdvanceGrants == 1
               && constrainedRecorder.timeAdvanceGrants == 1,
            "the lookahead boundary TAR did not complete for both federates");
         JavaTckSupport.check(epsilon.equals(regulator.queryLookahead()),
            "the deferred lower lookahead was not applied by the grant");

         constrained.disableTimeConstrained();
         constrainedRole = false;
         regulator.disableTimeRegulation();
         regulating = false;
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
