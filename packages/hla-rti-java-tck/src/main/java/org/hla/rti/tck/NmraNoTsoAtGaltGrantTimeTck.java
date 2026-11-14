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

/** Verifies an empty-input NMRA can grant at a request time equal to GALT. */
final class NmraNoTsoAtGaltGrantTimeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private NmraNoTsoAtGaltGrantTimeTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder regulatorCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-nmra-no-tso-at-galt-" + UUID.randomUUID();
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
         regulator.joinFederationExecution("java-tck-nmra-galt-regulator", federation);
         regulatorJoined = true;
         requester.joinFederationExecution("java-tck-nmra-galt-requester", federation);
         requesterJoined = true;

         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         JavaTckSupport.check(regulatorFactory.getName().equals(requesterFactory.getName()),
            "NMRA GALT members selected different logical-time factories");
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         JavaTckSupport.check(regulatorInitial.equals(requesterInitial),
            "NMRA GALT members began at different logical times");
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorFactory.makeEpsilon();
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(requester, callbackModel,
            () -> requesterCallbacks.timeConstrainedEnabled.size() == 1,
            "NMRA GALT requester Time Constrained callback");

         LogicalTime<?, ?> expectedGalt = offset(requesterInitial, requesterEpsilon, 5);
         LogicalTime<?, ?> expectedRegulatorGalt =
            offset(regulatorInitial, regulatorEpsilon, 5);
         JavaTckSupport.check(expectedGalt.equals(expectedRegulatorGalt),
            "NMRA GALT members derived different five-epsilon boundaries");
         LogicalTimeInterval regulatorLookahead =
            distance(expectedRegulatorGalt, regulatorInitial);
         regulator.enableTimeRegulation(regulatorLookahead);
         await(regulator, callbackModel,
            () -> regulatorCallbacks.timeRegulationEnabled.size() == 1,
            "NMRA GALT regulator Time Regulation callback");

         TimeQueryReturn observedGalt = requester.queryGALT();
         JavaTckSupport.check(observedGalt.timeIsValid
               && expectedGalt.equals(observedGalt.time),
            "NMRA requester did not observe the regulator's defined GALT");
         JavaTckSupport.check(requesterCallbacks.timeAdvanceGrants.isEmpty()
               && regulatorCallbacks.timeAdvanceGrants.isEmpty(),
            "NMRA GALT setup began with an unexpected grant callback");

         requester.nextMessageRequestAvailable(expectedGalt);
         await(requester, callbackModel,
            () -> requesterCallbacks.timeAdvanceGrants.size() == 1,
            "NMRA no-TSO grant at GALT request time");
         JavaTckSupport.check(requesterCallbacks.timeAdvanceGrants.size() == 1
               && expectedGalt.equals(requesterCallbacks.timeAdvanceGrants.get(0))
               && regulatorCallbacks.timeAdvanceGrants.isEmpty(),
            "NMRA with no TSO did not grant at request time without advancing the peer");
         JavaTckSupport.check(regulatorInitial.equals(regulator.queryLogicalTime()),
            "NMRA no-TSO grant changed regulator time");
         JavaTckSupport.check(expectedGalt.equals(requester.queryLogicalTime()),
            "NMRA no-TSO grant did not set requester time to the request time");
         TimeQueryReturn galtAfterGrant = requester.queryGALT();
         JavaTckSupport.check(galtAfterGrant.timeIsValid
               && expectedGalt.equals(galtAfterGrant.time),
            "NMRA no-TSO grant changed the defined GALT");
         JavaTckSupport.check(requesterCallbacks.timestampedInteractions.isEmpty(),
            "NMRA no-TSO grant unexpectedly delivered a timestamped interaction");

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
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval distance(LogicalTime end, LogicalTime start)
         throws Exception {
      return (LogicalTimeInterval) end.distance(start);
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Recorder {
      private final List<Object> timeConstrainedEnabled = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeRegulationEnabled = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timestampedInteractions =
         new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeConstrainedEnabled.add(arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeRegulationEnabled.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  timestampedInteractions.add((LogicalTime<?, ?>) arguments[6]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK NMRA GALT callbacks";
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
