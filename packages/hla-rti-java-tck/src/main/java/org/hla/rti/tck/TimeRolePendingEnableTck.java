package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.RequestForTimeConstrainedPending;
import hla.rti1516_2025.exceptions.RequestForTimeRegulationPending;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Checks the two callback-gated time-role enable transitions through the standard API. */
final class TimeRolePendingEnableTck {
   private TimeRolePendingEnableTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel model) throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_EVOKED,
         "pending time-role enablement is observed before evoking its callback");
      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-time-role-pending-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(recorder.proxy(), model);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("time-role-pending-member", federation);
         joined = true;

         LogicalTimeFactory<?, ?> timeFactory = ambassador.getTimeFactory();
         LogicalTime<?, ?> initial = ambassador.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> advance = afterEpsilon(initial, epsilon);

         ambassador.enableTimeRegulation(epsilon);
         JavaTckSupport.check(recorder.regulationEnabled == 0,
            "Time Regulation Enabled escaped before evoked callback service");
         expectPending(() -> ambassador.timeAdvanceRequest(advance),
            RequestForTimeRegulationPending.class, "TAR during pending Time Regulation");
         expectPending(() -> ambassador.enableTimeRegulation(epsilon),
            RequestForTimeRegulationPending.class, "repeated Enable Time Regulation");
         await(ambassador, () -> recorder.regulationEnabled == 1,
            "Time Regulation Enabled callback");
         JavaTckSupport.check(epsilon.equals(ambassador.queryLookahead()),
            "pending Time Regulation changed the requested lookahead");
         ambassador.disableTimeRegulation();

         ambassador.enableTimeConstrained();
         JavaTckSupport.check(recorder.constrainedEnabled == 0,
            "Time Constrained Enabled escaped before evoked callback service");
         expectPending(() -> ambassador.timeAdvanceRequest(advance),
            RequestForTimeConstrainedPending.class, "TAR during pending Time Constrained");
         expectPending(ambassador::enableTimeConstrained,
            RequestForTimeConstrainedPending.class, "repeated Enable Time Constrained");
         await(ambassador, () -> recorder.constrainedEnabled == 1,
            "Time Constrained Enabled callback");
         ambassador.disableTimeConstrained();

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         ambassador.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> afterEpsilon(LogicalTime initial,
         LogicalTimeInterval epsilon) throws Exception {
      return (LogicalTime<?, ?>) initial.add(epsilon);
   }

   private static void await(RTIambassador ambassador, BooleanSupplier ready,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!ready.getAsBoolean() && System.nanoTime() < deadline)
         ambassador.evokeCallback(0.0);
      JavaTckSupport.check(ready.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void expectPending(CheckedOperation operation,
         Class<? extends Exception> expected, String description) throws Exception {
      try {
         operation.run();
      } catch (Exception error) {
         JavaTckSupport.check(expected.isInstance(error),
            description + " threw " + error.getClass().getName()
               + " instead of " + expected.getName());
         return;
      }
      throw new AssertionError(description + " completed without a pending-role exception");
   }

   @FunctionalInterface
   private interface CheckedOperation { void run() throws Exception; }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("timeRegulationEnabled".equals(method.getName())) ++regulationEnabled;
               else if ("timeConstrainedEnabled".equals(method.getName())) ++constrainedEnabled;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK time-role callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
