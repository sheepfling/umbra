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

/** Verifies no-TSO NMRA remains pending until the regulating member advances GALT. */
final class NmraNoTsoWaitsForGaltTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;
   private static final long PREMATURE_GRANT_OBSERVATION_MILLIS = 250L;

   private NmraNoTsoWaitsForGaltTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder regulatorCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-nmra-no-tso-waits-for-galt-" + UUID.randomUUID();
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
         regulator.joinFederationExecution("java-tck-nmra-galt-wait-regulator", federation);
         regulatorJoined = true;
         requester.joinFederationExecution("java-tck-nmra-galt-wait-requester", federation);
         requesterJoined = true;

         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         JavaTckSupport.check(regulatorFactory.getName().equals(requesterFactory.getName()),
            "NMRA GALT-wait members selected different logical-time factories");
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         JavaTckSupport.check(regulatorInitial.equals(requesterInitial),
            "NMRA GALT-wait members began at different logical times");
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorFactory.makeEpsilon();
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();

         requester.enableTimeConstrained();
         await(regulator, requester, callbackModel,
            () -> requesterCallbacks.timeConstrainedEnabled.size() == 1,
            "NMRA GALT-wait requester Time Constrained callback");

         LogicalTime<?, ?> initialGalt = offset(regulatorInitial, regulatorEpsilon, 1);
         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 5);
         LogicalTime<?, ?> regulatorFirstTarget = offset(regulatorInitial, regulatorEpsilon, 3);
         LogicalTime<?, ?> galtAfterFirstAdvance = offset(requesterInitial, requesterEpsilon, 4);
         LogicalTime<?, ?> regulatorSecondTarget = offset(regulatorInitial, regulatorEpsilon, 4);
         LogicalTime<?, ?> expectedGalt = offset(requesterInitial, requesterEpsilon, 5);
         JavaTckSupport.check(requestTime.equals(expectedGalt),
            "NMRA request and final GALT boundaries differ");

         regulator.enableTimeRegulation(regulatorEpsilon);
         await(regulator, requester, callbackModel,
            () -> regulatorCallbacks.timeRegulationEnabled.size() == 1,
            "NMRA GALT-wait regulator Time Regulation callback");

         requireGalt(requester, initialGalt,
            "NMRA GALT-wait requester did not observe initial GALT below the request");
         JavaTckSupport.check(regulatorCallbacks.timeAdvanceGrants.isEmpty()
               && requesterCallbacks.timeAdvanceGrants.isEmpty(),
            "NMRA GALT-wait setup began with an unexpected grant callback");

         requester.nextMessageRequestAvailable(requestTime);
         requireNoPrematureGrant(regulator, requester, callbackModel, requesterCallbacks,
            "NMRA without TSO granted while initial GALT was below the request");

         regulator.timeAdvanceRequest(regulatorFirstTarget);
         await(regulator, requester, callbackModel,
            () -> regulatorCallbacks.timeAdvanceGrants.size() == 1,
            "NMRA GALT-wait regulator first advance");
         JavaTckSupport.check(regulatorFirstTarget.equals(
               regulatorCallbacks.timeAdvanceGrants.get(0)),
            "NMRA GALT-wait regulator did not reach its first target");
         requireGalt(requester, galtAfterFirstAdvance,
            "NMRA GALT-wait requester did not observe intermediate GALT below the request");
         requireNoPrematureGrant(regulator, requester, callbackModel, requesterCallbacks,
            "NMRA without TSO granted while intermediate GALT remained below the request");

         regulator.timeAdvanceRequest(regulatorSecondTarget);
         await(regulator, requester, callbackModel,
            () -> regulatorCallbacks.timeAdvanceGrants.size() == 2,
            "NMRA GALT-wait regulator second advance");
         JavaTckSupport.check(regulatorSecondTarget.equals(
               regulatorCallbacks.timeAdvanceGrants.get(1)),
            "NMRA GALT-wait regulator did not reach its second target");
         requireGalt(requester, expectedGalt,
            "NMRA GALT-wait requester did not observe request-time GALT");

         await(regulator, requester, callbackModel,
            () -> requesterCallbacks.timeAdvanceGrants.size() == 1,
            "NMRA no-TSO grant when GALT reaches request time");
         JavaTckSupport.check(requestTime.equals(requesterCallbacks.timeAdvanceGrants.get(0))
               && requesterCallbacks.timestampedInteractions.isEmpty(),
            "NMRA without TSO did not grant at request time when GALT reached it");
         JavaTckSupport.check(regulatorSecondTarget.equals(regulator.queryLogicalTime()),
            "NMRA GALT-wait regulator current time is incorrect");
         JavaTckSupport.check(requestTime.equals(requester.queryLogicalTime()),
            "NMRA GALT-wait requester current time is incorrect");
         JavaTckSupport.check(requesterCallbacks.timeAdvanceGrants.size() == 1
               && requesterCallbacks.timestampedInteractions.isEmpty(),
            "NMRA GALT-wait produced duplicate grants or an unexpected TSO");

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

   private static void requireGalt(RTIambassador ambassador,
         LogicalTime<?, ?> expected, String description) throws Exception {
      TimeQueryReturn observed = ambassador.queryGALT();
      JavaTckSupport.check(observed.timeIsValid && expected.equals(observed.time), description);
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

   private static void requireNoPrematureGrant(RTIambassador regulator,
         RTIambassador requester, CallbackModel callbackModel,
         Recorder requesterCallbacks, String description) throws Exception {
      long deadline = System.nanoTime()
         + PREMATURE_GRANT_OBSERVATION_MILLIS * 1_000_000L;
      while (System.nanoTime() < deadline) {
         serviceCallbacks(regulator, requester, callbackModel);
         Thread.sleep(1L);
      }
      JavaTckSupport.check(requesterCallbacks.timeAdvanceGrants.isEmpty()
            && requesterCallbacks.timestampedInteractions.isEmpty(), description);
   }

   private static void await(RTIambassador regulator, RTIambassador requester,
         CallbackModel callbackModel, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         serviceCallbacks(regulator, requester, callbackModel);
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static void serviceCallbacks(RTIambassador regulator,
         RTIambassador requester, CallbackModel callbackModel) throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         regulator.evokeCallback(0.0);
         requester.evokeCallback(0.0);
      } else {
         Thread.yield();
      }
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
      private final List<LogicalTime<?, ?>> timeRegulationEnabled =
         new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrants =
         new CopyOnWriteArrayList<>();
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
                  if ("toString".equals(name)) return "Java TCK NMRA GALT-wait callbacks";
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
