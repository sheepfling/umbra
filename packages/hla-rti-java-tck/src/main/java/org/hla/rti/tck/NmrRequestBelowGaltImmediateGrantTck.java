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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks immediate NMR grant when the request is strictly below defined GALT. */
final class NmrRequestBelowGaltImmediateGrantTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private NmrRequestBelowGaltImmediateGrantTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder regulatorCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-nmr-below-galt-" + UUID.randomUUID();
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
         regulator.joinFederationExecution("java-tck-nmr-galt-regulator", federation);
         regulatorJoined = true;
         requester.joinFederationExecution("java-tck-nmr-galt-requester", federation);
         requesterJoined = true;

         LogicalTimeFactory<?, ?> regulatorTimeFactory = regulator.getTimeFactory();
         LogicalTimeFactory<?, ?> requesterTimeFactory = requester.getTimeFactory();
         JavaTckSupport.check(regulatorTimeFactory.getName().equals(requesterTimeFactory.getName()),
            "NMR below-GALT members selected different logical-time factories");
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         JavaTckSupport.check(regulatorInitial.equals(requesterInitial),
            "NMR below-GALT members began at different logical times");
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorTimeFactory.makeEpsilon();
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterTimeFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(requester, callbackModel, () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "NMR below-GALT requester Time Constrained callback");

         LogicalTimeInterval requestedLookahead = offset(regulatorEpsilon, 5);
         regulator.enableTimeRegulation(requestedLookahead);
         await(regulator, callbackModel, () -> regulatorCallbacks.timeRegulationEnabled == 1,
            "NMR below-GALT regulator Time Regulation callback");
         LogicalTimeInterval actualLookahead = regulator.queryLookahead();
         JavaTckSupport.check(requestedLookahead.equals(actualLookahead),
            "NMR below-GALT regulator did not retain the configured lookahead");

         LogicalTime<?, ?> expectedGalt = offset(regulatorInitial, regulatorEpsilon, 5);
         TimeQueryReturn beforeRequestGalt = requester.queryGALT();
         JavaTckSupport.check(beforeRequestGalt.timeIsValid
               && expectedGalt.equals(beforeRequestGalt.time),
            "NMR below-GALT requester did not observe the regulator's defined GALT");
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 3);
         JavaTckSupport.check(!requestTime.equals(expectedGalt),
            "NMR below-GALT request unexpectedly equals GALT");
         JavaTckSupport.check(regulatorCallbacks.grants.isEmpty()
               && requesterCallbacks.grants.isEmpty(),
            "NMR below-GALT test began with an unexpected grant callback");

         requester.nextMessageRequest(requestTime);
         await(requester, callbackModel, () -> requesterCallbacks.grants.size() == 1,
            "NMR request strictly below GALT grant");
         JavaTckSupport.check(requesterCallbacks.grants.size() == 1
               && requestTime.equals(requesterCallbacks.grants.get(0)),
            "NMR request below GALT did not grant exactly once at its requested time");
         JavaTckSupport.check(regulatorCallbacks.grants.isEmpty(),
            "NMR below-GALT request required or produced a regulator grant");
         JavaTckSupport.check(regulatorInitial.equals(regulator.queryLogicalTime()),
            "NMR below-GALT regulator advanced despite no time-advance request");
         JavaTckSupport.check(requestTime.equals(requester.queryLogicalTime()),
            "NMR below-GALT requester query returned the wrong logical time");
         TimeQueryReturn afterGrantGalt = requester.queryGALT();
         JavaTckSupport.check(afterGrantGalt.timeIsValid
               && expectedGalt.equals(afterGrantGalt.time),
            "NMR below-GALT grant changed the defined regulator boundary");

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
   private static LogicalTimeInterval offset(LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTimeInterval result = interval;
      for (int index = 1; index < steps; ++index) {
         result = (LogicalTimeInterval) result.add(interval);
      }
      return result;
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
      private volatile int timeConstrainedEnabled;
      private volatile int timeRegulationEnabled;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled++;
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK NMR below-GALT callbacks";
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
