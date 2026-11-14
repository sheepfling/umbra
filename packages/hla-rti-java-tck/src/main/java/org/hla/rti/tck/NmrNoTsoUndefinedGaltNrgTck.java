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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Covers no-TSO NMR under undefined GALT, both NRG settings, and two-regulator blocking. */
final class NmrNoTsoUndefinedGaltNrgTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long QUIET_OBSERVATION_NANOS = 250_000_000L;

   private NmrNoTsoUndefinedGaltNrgTck() { }

   static void run(RtiFactory factory, String nrgDisabledFom, String nrgEnabledFom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      runOne(factory, nrgDisabledFom, timeImplementation, callbackModel, false);
      runOne(factory, nrgEnabledFom, timeImplementation, callbackModel, true);
   }

   private static void runOne(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel, boolean expectedNrgEnabled)
         throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador secondRegulator = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder regulatorCallbacks = new Recorder();
      Recorder secondRegulatorCallbacks = new Recorder();
      String federation = "java-tck-nmr-no-tso-undefined-galt-"
         + (expectedNrgEnabled ? "nrg-enabled-" : "nrg-disabled-") + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean regulatorConnected = false;
      boolean secondRegulatorConnected = false;
      boolean federationCreated = false;
      boolean requesterJoined = false;
      boolean regulatorJoined = false;
      boolean secondRegulatorJoined = false;
      boolean requesterConstrained = false;
      boolean regulatorRegulating = false;
      boolean secondRegulatorRegulating = false;
      try {
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         regulatorConnected = true;
         secondRegulator.connect(secondRegulatorCallbacks.proxy(), callbackModel);
         secondRegulatorConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         requester.joinFederationExecution("java-tck-nmr-no-tso-requester", federation);
         requesterJoined = true;
         regulator.joinFederationExecution("java-tck-nmr-no-tso-regulator", federation);
         regulatorJoined = true;
         secondRegulator.joinFederationExecution(
            "java-tck-nmr-no-tso-second-regulator", federation);
         secondRegulatorJoined = true;

         boolean nrgEnabled = requester.getNonRegulatedGrantSwitch();
         JavaTckSupport.check(nrgEnabled == expectedNrgEnabled
               && regulator.getNonRegulatedGrantSwitch() == nrgEnabled
               && secondRegulator.getNonRegulatedGrantSwitch() == nrgEnabled,
            "NMR no-TSO adapter FOM did not match the expected NRG switch");
         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         LogicalTimeFactory<?, ?> secondRegulatorFactory = secondRegulator.getTimeFactory();
         JavaTckSupport.check(requesterFactory.getName().equals(regulatorFactory.getName())
               && requesterFactory.getName().equals(secondRegulatorFactory.getName()),
            "NMR no-TSO members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> secondRegulatorInitial = secondRegulator.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(regulatorInitial)
               && requesterInitial.equals(secondRegulatorInitial),
            "NMR no-TSO members began at different logical times");
         LogicalTimeInterval<?> requesterEpsilon = requesterFactory.makeEpsilon();
         LogicalTimeInterval<?> regulatorEpsilon = regulatorFactory.makeEpsilon();
         LogicalTimeInterval<?> secondRegulatorEpsilon = secondRegulatorFactory.makeEpsilon();

         requester.enableTimeConstrained();
         requesterConstrained = true;
         await(requester, callbackModel,
            () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "NMR no-TSO requester Time Constrained callback");
         regulator.enableTimeRegulation(regulatorEpsilon);
         regulatorRegulating = true;
         await(regulator, callbackModel,
            () -> regulatorCallbacks.timeRegulationEnabled == 1,
            "NMR no-TSO regulator Time Regulation callback");
         secondRegulator.enableTimeRegulation(secondRegulatorEpsilon);
         await(secondRegulator, callbackModel,
            () -> secondRegulatorCallbacks.timeRegulationEnabled == 1,
            "NMR no-TSO second regulator Time Regulation callback");

         LogicalTime<?, ?> initialGalt = offset(requesterInitial, requesterEpsilon, 1);
         TimeQueryReturn definedGalt = requester.queryGALT();
         JavaTckSupport.check(definedGalt != null && definedGalt.timeIsValid
               && initialGalt.equals(definedGalt.time),
            "NMR no-TSO setup did not establish the expected initial GALT");
         regulator.disableTimeRegulation();
         regulatorRegulating = false;
         secondRegulator.disableTimeRegulation();
         secondRegulatorRegulating = false;
         settleCallbacks(requester, regulator, secondRegulator, callbackModel,
            QUIET_OBSERVATION_NANOS);
         TimeQueryReturn undefinedGalt = requester.queryGALT();
         JavaTckSupport.check(undefinedGalt != null && !undefinedGalt.timeIsValid,
            "NMR no-TSO setup unexpectedly retained a defined GALT");

         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         requester.nextMessageRequest(requestTime);
         if (!expectedNrgEnabled) {
            settleCallbacks(requester, regulator, secondRegulator, callbackModel,
               QUIET_OBSERVATION_NANOS);
            JavaTckSupport.check(requesterCallbacks.interactionCallbacks == 0
                  && requesterCallbacks.grants.isEmpty()
                  && requester.queryLogicalTime().equals(requesterInitial),
               "NMR with undefined GALT and NRG disabled advanced before regulation resumed");

            regulator.enableTimeRegulation(regulatorEpsilon);
            regulatorRegulating = true;
            await(regulator, callbackModel,
               () -> regulatorCallbacks.timeRegulationEnabled == 2,
               "NMR no-TSO resumed regulator Time Regulation callback");
            secondRegulator.enableTimeRegulation(secondRegulatorEpsilon);
            secondRegulatorRegulating = true;
            await(secondRegulator, callbackModel,
               () -> secondRegulatorCallbacks.timeRegulationEnabled == 2,
               "NMR no-TSO resumed second regulator Time Regulation callback");
            LogicalTime<?, ?> regulatorTarget = offset(regulatorInitial, regulatorEpsilon, 11);
            regulator.timeAdvanceRequest(regulatorTarget);
            await(regulator, callbackModel, () -> regulatorCallbacks.grants.size() == 1,
               "NMR no-TSO resumed regulator advance");
            JavaTckSupport.check(regulatorCallbacks.grants.size() == 1
                  && regulatorTarget.equals(regulatorCallbacks.grants.get(0)),
               "NMR no-TSO regulator did not reach time 11");

            settleCallbacks(requester, regulator, secondRegulator, callbackModel,
               QUIET_OBSERVATION_NANOS);
            JavaTckSupport.check(requesterCallbacks.grants.isEmpty()
                  && requesterCallbacks.interactionCallbacks == 0
                  && requester.queryLogicalTime().equals(requesterInitial),
               "NMR granted while the second regulator still limited GALT");
            TimeQueryReturn galtWhileSecondRegulatorIsBehind = requester.queryGALT();
            JavaTckSupport.check(galtWhileSecondRegulatorIsBehind != null
                  && galtWhileSecondRegulatorIsBehind.timeIsValid
                  && initialGalt.equals(galtWhileSecondRegulatorIsBehind.time),
               "NMR GALT changed after only the primary regulator advanced");

            LogicalTime<?, ?> secondRegulatorTarget =
               offset(secondRegulatorInitial, secondRegulatorEpsilon, 11);
            secondRegulator.timeAdvanceRequest(secondRegulatorTarget);
            await(secondRegulator, callbackModel,
               () -> secondRegulatorCallbacks.grants.size() == 1,
               "NMR no-TSO second regulator advance");
            JavaTckSupport.check(secondRegulatorCallbacks.grants.size() == 1
                  && secondRegulatorTarget.equals(secondRegulatorCallbacks.grants.get(0)),
               "NMR no-TSO second regulator did not reach time 11");
            TimeQueryReturn galtAfterBothRegulatorsAdvance = requester.queryGALT();
            LogicalTime<?, ?> requestClearedGalt = offset(requesterInitial, requesterEpsilon, 12);
            JavaTckSupport.check(galtAfterBothRegulatorsAdvance != null
                  && galtAfterBothRegulatorsAdvance.timeIsValid
                  && requestClearedGalt.equals(galtAfterBothRegulatorsAdvance.time),
               "NMR GALT did not clear the request after both regulators advanced");
         }

         awaitBoth(requester, regulator, secondRegulator, callbackModel,
            () -> requesterCallbacks.grants.size() == 1,
            "NMR no-TSO grant at the requested time");
         JavaTckSupport.check(requesterCallbacks.interactionCallbacks == 0
               && requesterCallbacks.grants.size() == 1
               && requesterCallbacks.callbackOrder.equals(Arrays.asList("grant"))
               && requestTime.equals(requesterCallbacks.grants.get(0))
               && requestTime.equals(requester.queryLogicalTime()),
            "NMR no-TSO did not grant exactly once at its request time without a TSO");
         JavaTckSupport.check(regulatorCallbacks.grants.size()
               == (expectedNrgEnabled ? 0 : 1),
            "NMR no-TSO used the wrong number of regulator advances");
         JavaTckSupport.check(secondRegulatorCallbacks.grants.size()
               == (expectedNrgEnabled ? 0 : 1),
            "NMR no-TSO used the wrong number of second-regulator advances");
         if (expectedNrgEnabled) {
            TimeQueryReturn finalGalt = requester.queryGALT();
            JavaTckSupport.check(finalGalt != null && !finalGalt.timeIsValid,
               "NMR no-TSO NRG-enabled grant unexpectedly defined GALT");
         }

         requester.disableTimeConstrained();
         requesterConstrained = false;
         if (regulatorRegulating) {
            regulator.disableTimeRegulation();
            regulatorRegulating = false;
         }
         if (secondRegulatorRegulating) {
            secondRegulator.disableTimeRegulation();
            secondRegulatorRegulating = false;
         }
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         secondRegulator.resignFederationExecution(ResignAction.NO_ACTION);
         secondRegulatorJoined = false;
         requester.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (requesterConstrained && requesterJoined) {
            try { requester.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulatorRegulating && regulatorJoined) {
            try { regulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (secondRegulatorRegulating && secondRegulatorJoined) {
            try { secondRegulator.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (requesterJoined) resign(requester);
         if (regulatorJoined) resign(regulator);
         if (secondRegulatorJoined) resign(secondRegulator);
         if (federationCreated) {
            try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (regulatorConnected) disconnect(regulator);
         if (secondRegulatorConnected) disconnect(secondRegulator);
      }
   }

   private static void settleCallbacks(RTIambassador requester, RTIambassador regulator,
         RTIambassador secondRegulator, CallbackModel model, long durationNanos)
         throws Exception {
      long deadline = System.nanoTime() + durationNanos;
      while (System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            requester.evokeCallback(0.0);
            regulator.evokeCallback(0.0);
            secondRegulator.evokeCallback(0.0);
         } else {
            Thread.sleep(1L);
         }
      }
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

   private static void awaitBoth(RTIambassador requester, RTIambassador regulator,
         RTIambassador secondRegulator, CallbackModel model, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            requester.evokeCallback(0.0);
            regulator.evokeCallback(0.0);
            secondRegulator.evokeCallback(0.0);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
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
      private volatile int interactionCallbacks;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled++;
               } else if ("receiveInteraction".equals(name)) {
                  interactionCallbacks++;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK NMR no-TSO callbacks";
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
