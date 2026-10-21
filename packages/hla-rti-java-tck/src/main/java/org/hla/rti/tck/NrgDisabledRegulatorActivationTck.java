package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** A constrained TAR waits with NRG disabled until a regulator becomes active. */
final class NrgDisabledRegulatorActivationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NrgDisabledRegulatorActivationTck() { }

   static void run(RtiFactory factory, String nrgDisabledFom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador regulator = factory.getRtiAmbassador();
      Recorder receiverCallbacks = new Recorder();
      Recorder regulatorCallbacks = new Recorder();
      String federation = "java-tck-nrg-disabled-activation-" + UUID.randomUUID();
      boolean receiverConnected = false;
      boolean regulatorConnected = false;
      boolean created = false;
      boolean receiverJoined = false;
      boolean regulatorJoined = false;
      try {
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         regulator.connect(regulatorCallbacks.proxy(), callbackModel);
         regulatorConnected = true;
         receiver.createFederationExecution(federation, nrgDisabledFom, timeImplementation);
         created = true;
         receiver.joinFederationExecution("java-tck-nrg-disabled-receiver", federation);
         receiverJoined = true;
         regulator.joinFederationExecution("java-tck-nrg-disabled-regulator", federation);
         regulatorJoined = true;
         JavaTckSupport.check(!receiver.getNonRegulatedGrantSwitch()
               && !regulator.getNonRegulatedGrantSwitch(),
            "adapter FOM did not disable the standard non-regulated grant switch");

         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> regulatorFactory = regulator.getTimeFactory();
         JavaTckSupport.check(receiverFactory.getName().equals(regulatorFactory.getName()),
            "NRG-disabled participants selected different logical-time factories");
         LogicalTime<?, ?> initial = receiver.queryLogicalTime();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         LogicalTimeInterval regulatorEpsilon =
            (LogicalTimeInterval) regulatorFactory.makeEpsilon();
         LogicalTime<?, ?> target = offset(initial, receiverEpsilon);
         LogicalTimeInterval lookahead = twice(regulatorEpsilon);

         receiver.enableTimeConstrained();
         await(receiver, callbackModel, () -> receiverCallbacks.constrainedEnabled == 1,
            "NRG-disabled receiver time-constrained enablement");
         receiver.timeAdvanceRequest(target);
         if (callbackModel == CallbackModel.HLA_EVOKED) receiver.evokeCallback(0.0);
         JavaTckSupport.check(receiverCallbacks.grants.isEmpty(),
            "NRG-disabled constrained TAR was granted without a regulator");

         regulator.enableTimeRegulation(lookahead);
         await(regulator, callbackModel, () -> regulatorCallbacks.regulationEnabled == 1,
            "NRG-disabled regulator enablement");
         await(receiver, callbackModel, () -> receiverCallbacks.grants.size() == 1,
            "NRG-disabled grant after regulator activation");
         JavaTckSupport.check(receiverCallbacks.grants.size() == 1
               && target.equals(receiverCallbacks.grants.get(0)),
            "regulator activation did not grant the pending TAR at its requested time");

         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         receiver.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (regulatorJoined) resign(regulator);
         if (receiverJoined) resign(receiver);
         if (created) {
            try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (regulatorConnected) disconnect(regulator);
         if (receiverConnected) disconnect(receiver);
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval) throws Exception {
      return (LogicalTime) start.add(interval);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval twice(LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTimeInterval) interval.add(interval);
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
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("timeConstrainedEnabled".equals(name)) constrainedEnabled++;
               else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK NRG-disabled callbacks";
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
