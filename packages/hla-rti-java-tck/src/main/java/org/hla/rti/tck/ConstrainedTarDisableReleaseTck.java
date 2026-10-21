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

/** A pending constrained TAR is released when that federate disables constraint. */
final class ConstrainedTarDisableReleaseTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ConstrainedTarDisableReleaseTck() { }

   static void run(RtiFactory factory, String nrgDisabledFom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-constrained-tar-disable-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         receiver.connect(callbacks.proxy(), callbackModel);
         connected = true;
         receiver.createFederationExecution(federation, nrgDisabledFom, timeImplementation);
         created = true;
         receiver.joinFederationExecution("java-tck-constrained-tar-receiver", federation);
         joined = true;
         JavaTckSupport.check(!receiver.getNonRegulatedGrantSwitch(),
            "adapter FOM did not disable the standard non-regulated grant switch");

         LogicalTimeFactory<?, ?> timeFactory = receiver.getTimeFactory();
         LogicalTime<?, ?> initial = receiver.queryLogicalTime();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime<?, ?> target = offset(initial, epsilon);

         receiver.enableTimeConstrained();
         await(receiver, callbackModel, () -> callbacks.constrainedEnabled == 1,
            "constrained TAR receiver role enablement");
         receiver.timeAdvanceRequest(target);
         if (callbackModel == CallbackModel.HLA_EVOKED) receiver.evokeCallback(0.0);
         JavaTckSupport.check(callbacks.grants.isEmpty(),
            "NRG-disabled constrained TAR was granted without a regulator");

         receiver.disableTimeConstrained();
         await(receiver, callbackModel, () -> callbacks.grants.size() == 1,
            "grant after disabling time-constrained role");
         JavaTckSupport.check(callbacks.grants.size() == 1
               && target.equals(callbacks.grants.get(0)),
            "disabling the constrained role did not grant the pending TAR at its requested time");

         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         receiver.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval) throws Exception {
      return (LogicalTime) start.add(interval);
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

   private static final class Recorder {
      private volatile int constrainedEnabled;
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) constrainedEnabled++;
               else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK constrained TAR callbacks";
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
