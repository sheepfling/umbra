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

/** Standard-API checks for delayed time-constrained enablement at GALT equality. */
final class TimeConstrainedEnableGaltEqualityTck {
   private TimeConstrainedEnableGaltEqualityTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder regulatorRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-time-constrained-galt-equality-" + UUID.randomUUID();
      boolean regulatorConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean regulatorJoined = false;
      boolean requesterJoined = false;
      try {
         regulator.connect(regulatorRecorder.proxy(), callbackModel);
         regulatorConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         regulator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         regulator.joinFederationExecution("java-tck-galt-regulator", federation);
         regulatorJoined = true;
         requester.joinFederationExecution("java-tck-galt-requester", federation);
         requesterJoined = true;

         LogicalTimeFactory<?, ?> timeFactory = regulator.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(requester.getTimeFactory().getName()),
            "time-constrained GALT members selected different logical-time factories");
         LogicalTime<?, ?> initial = (LogicalTime<?, ?>) timeFactory.makeInitial();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         regulator.enableTimeRegulation(epsilon);
         await(callbackModel, regulator,
            () -> regulatorRecorder.timeRegulationEnabled == 1,
            "regulator time-regulation callback");

         LogicalTime<?, ?> five = add(initial, offset(epsilon, 5));
         requester.timeAdvanceRequest(five);
         await(callbackModel, requester,
            () -> five.equals(requesterRecorder.timeAdvanceGrant),
            "unconstrained requester grant to five");

         LogicalTime<?, ?> one = add(initial, epsilon);
         TimeQueryReturn galt = requester.queryGALT();
         JavaTckSupport.check(galt.timeIsValid && one.equals(galt.time),
            "GALT before constrained enablement was not one");

         requester.enableTimeConstrained();
         if (callbackModel == CallbackModel.HLA_EVOKED) requester.evokeCallback(0.0);
         JavaTckSupport.check(requesterRecorder.timeConstrainedEnabled == 0,
            "time-constrained enable completed before GALT reached current time");

         LogicalTime<?, ?> four = add(initial, offset(epsilon, 4));
         regulator.timeAdvanceRequest(four);
         await(callbackModel, regulator,
            () -> four.equals(regulatorRecorder.timeAdvanceGrant),
            "regulator grant to four");
         TimeQueryReturn equalityGalt = requester.queryGALT();
         JavaTckSupport.check(equalityGalt.timeIsValid && five.equals(equalityGalt.time),
            "GALT did not equal current logical time before constrained enablement");
         LogicalTime<?, ?> regulatorFive = add(initial, offset(epsilon, 5));
         regulator.timeAdvanceRequest(regulatorFive);
         await(callbackModel, regulator,
            () -> regulatorFive.equals(regulatorRecorder.timeAdvanceGrant),
            "subsequent regulator grant to five");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(requesterRecorder.timeConstrainedEnabled == 0,
               "time-constrained callback was delivered without requester callback servicing");
         }
         await(callbackModel, requester,
            () -> requesterRecorder.timeConstrainedEnabled == 1,
            "time-constrained callback at GALT equality");
         JavaTckSupport.check(five.equals(requesterRecorder.timeConstrainedTime),
            "time-constrained callback reported a time other than five");
         JavaTckSupport.check(five.equals(requester.queryLogicalTime()),
            "time-constrained enablement did not set the callback's logical time");

         LogicalTime<?, ?> six = add(initial, offset(epsilon, 6));
         LogicalTime<?, ?> eight = add(initial, offset(epsilon, 8));
         TimeQueryReturn heldGalt = requester.queryGALT();
         JavaTckSupport.check(heldGalt.timeIsValid && six.equals(heldGalt.time),
            "regulator-constrained advance had the wrong GALT before the request");
         requester.timeAdvanceRequest(eight);
         long heldDeadline = System.nanoTime() + 50_000_000L;
         while (System.nanoTime() < heldDeadline) {
            if (callbackModel == CallbackModel.HLA_EVOKED) requester.evokeCallback(0.0);
            JavaTckSupport.check(five.equals(requesterRecorder.timeAdvanceGrant),
               "time-constrained requester advanced beyond the regulating peer's GALT");
            Thread.yield();
         }
         JavaTckSupport.check(five.equals(requester.queryLogicalTime()),
            "regulator-constrained request changed logical time before the peer advanced");
         heldGalt = requester.queryGALT();
         JavaTckSupport.check(heldGalt.timeIsValid && six.equals(heldGalt.time),
            "GALT changed while the constrained advance was held");
         regulator.timeAdvanceRequest(eight);
         await(callbackModel, regulator,
            () -> eight.equals(regulatorRecorder.timeAdvanceGrant),
            "regulator grant that releases the constrained requester");
         await(callbackModel, requester,
            () -> eight.equals(requesterRecorder.timeAdvanceGrant),
            "time-constrained requester grant after regulator advance");
         JavaTckSupport.check(eight.equals(requester.queryLogicalTime()),
            "time-constrained requester did not reach its granted time");

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

   private static LogicalTime add(LogicalTime time, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) time.add(interval);
   }

   private static LogicalTimeInterval offset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
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
   private interface Condition { boolean isMet() throws Exception; }

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
      private volatile LogicalTime<?, ?> timeConstrainedTime;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled++;
               if ("timeConstrainedEnabled".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeConstrainedEnabled++;
                  timeConstrainedTime = (LogicalTime<?, ?>) arguments[0];
               }
               if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK GALT equality callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
