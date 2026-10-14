package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TimeQueryReturn;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Standard-API checks for GALT/LITS boundaries as time regulators advance. */
final class TimeBoundsQueriesTck {
   private TimeBoundsQueriesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador left = factory.getRtiAmbassador();
      RTIambassador right = factory.getRtiAmbassador();
      Recorder observerRecorder = new Recorder();
      Recorder leftRecorder = new Recorder();
      Recorder rightRecorder = new Recorder();
      String federation = "java-tck-time-bounds-queries-" + UUID.randomUUID();
      boolean observerConnected = false;
      boolean leftConnected = false;
      boolean rightConnected = false;
      boolean created = false;
      boolean observerJoined = false;
      boolean leftJoined = false;
      boolean rightJoined = false;
      try {
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         left.connect(leftRecorder.proxy(), callbackModel);
         leftConnected = true;
         right.connect(rightRecorder.proxy(), callbackModel);
         rightConnected = true;
         observer.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         observer.joinFederationExecution("java-tck-time-bounds-observer", federation);
         observerJoined = true;
         left.joinFederationExecution("java-tck-time-bounds-left-regulator", federation);
         leftJoined = true;
         right.joinFederationExecution("java-tck-time-bounds-right-regulator", federation);
         rightJoined = true;

         LogicalTimeFactory<?, ?> factoryForTime = observer.getTimeFactory();
         LogicalTime<?, ?> initial = (LogicalTime<?, ?>) factoryForTime.makeInitial();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) factoryForTime.makeEpsilon();
         JavaTckSupport.check(factoryForTime.getName().equals(left.getTimeFactory().getName())
               && factoryForTime.getName().equals(right.getTimeFactory().getName()),
            "time-management members selected different logical-time factories");

         expectUndefined(observer, "GALT before any other regulator is active", true);
         expectUndefined(observer, "LITS before any other regulator is active", false);

         LogicalTimeInterval leftLookahead = offset(epsilon, 1);
         left.enableTimeRegulation(leftLookahead);
         await(callbackModel, left, () -> leftRecorder.timeRegulationEnabled == 1,
            "first time-regulation callback");
         LogicalTime<?, ?> one = add(initial, epsilon);
         expectBoundary(observer, one, "first-regulator GALT", true);
         expectBoundary(observer, one, "first-regulator LITS", false);

         right.enableTimeRegulation(offset(epsilon, 3));
         await(callbackModel, right, () -> rightRecorder.timeRegulationEnabled == 1,
            "second time-regulation callback");
         expectBoundary(observer, one, "minimum GALT across regulators", true);
         expectBoundary(observer, one, "minimum LITS across regulators", false);

         LogicalTime<?, ?> leftTarget = add(initial, offset(epsilon, 4));
         LogicalTime<?, ?> rightTarget = add(initial, epsilon);
         left.timeAdvanceRequest(leftTarget);
         right.timeAdvanceRequest(rightTarget);
         LogicalTime<?, ?> four = add(initial, offset(epsilon, 4));
         expectBoundary(observer, four, "GALT while regulator advances are pending", true);
         expectBoundary(observer, four, "LITS while regulator advances are pending", false);

         await(callbackModel, left, () -> leftRecorder.timeAdvanceGrant != null,
            "left regulator time-advance grant");
         await(callbackModel, right, () -> rightRecorder.timeAdvanceGrant != null,
            "right regulator time-advance grant");
         JavaTckSupport.check(leftTarget.equals(leftRecorder.timeAdvanceGrant)
               && rightTarget.equals(rightRecorder.timeAdvanceGrant),
            "pending regulator advances returned incorrect grant times");
         expectBoundary(observer, four, "GALT after regulator grants", true);
         expectBoundary(observer, four, "LITS after regulator grants", false);

         right.resignFederationExecution(ResignAction.NO_ACTION);
         rightJoined = false;
         LogicalTime<?, ?> five = add(initial, offset(epsilon, 5));
         expectBoundary(observer, five, "GALT after the minimum regulator resigns", true);
         expectBoundary(observer, five, "LITS after the minimum regulator resigns", false);

         left.disableTimeRegulation();
         expectUndefined(observer, "GALT after every other regulator is disabled", true);
         expectUndefined(observer, "LITS after every other regulator is disabled", false);

         left.resignFederationExecution(ResignAction.NO_ACTION);
         leftJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         JavaTckSupport.expectFailure(observer::queryGALT,
            "FederateNotExecutionMember", "Query GALT after observer resignation");
         JavaTckSupport.expectFailure(observer::queryLITS,
            "FederateNotExecutionMember", "Query LITS after observer resignation");

         observer.destroyFederationExecution(federation);
         created = false;
         right.disconnect();
         rightConnected = false;
         left.disconnect();
         leftConnected = false;
         observer.disconnect();
         observerConnected = false;
         JavaTckSupport.expectFailure(observer::queryGALT, "NotConnected",
            "Query GALT after observer disconnect");
         JavaTckSupport.expectFailure(observer::queryLITS, "NotConnected",
            "Query LITS after observer disconnect");
      } finally {
         if (rightJoined) resign(right);
         if (leftJoined) resign(left);
         if (observerJoined) resign(observer);
         if (created) {
            try { observer.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (rightConnected) disconnect(right);
         if (leftConnected) disconnect(left);
         if (observerConnected) disconnect(observer);
      }
   }

   private static void expectUndefined(RTIambassador ambassador, String description,
         boolean galt) throws Exception {
      TimeQueryReturn result = galt ? ambassador.queryGALT() : ambassador.queryLITS();
      JavaTckSupport.check(!result.timeIsValid,
         description + " unexpectedly returned a defined bound");
   }

   private static void expectBoundary(RTIambassador ambassador, LogicalTime<?, ?> expected,
         String description, boolean galt) throws Exception {
      TimeQueryReturn result = galt ? ambassador.queryGALT() : ambassador.queryLITS();
      JavaTckSupport.check(result.timeIsValid,
         description + " unexpectedly returned an undefined bound");
      JavaTckSupport.check(expected.equals(result.time),
         description + " returned an incorrect logical time");
   }

   private static LogicalTime add(LogicalTime time,
         LogicalTimeInterval interval) throws Exception {
      return (LogicalTime) time.add(interval);
   }

   private static LogicalTimeInterval offset(LogicalTimeInterval epsilon,
         int steps) throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) {
         result = (LogicalTimeInterval) result.add(epsilon);
      }
      return result;
   }

   private static void await(CallbackModel callbackModel, RTIambassador ambassador,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         if (condition.isMet()) return;
         Thread.yield();
      }
      throw new AssertionError(description + " did not arrive before timeout");
   }

   @FunctionalInterface
   private interface Condition {
      boolean isMet() throws Exception;
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
      private volatile LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled++;
               if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK time-bounds callbacks";
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
