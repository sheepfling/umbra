package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Replays an accepted TimeAdvanceRequestAvailable response after an adapter-managed fault. */
final class ConnectionFaultTimeAdvanceRequestAvailableResponseReplayTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long CALLBACK_SETTLE_NANOS = 20_000_000L;

   private ConnectionFaultTimeAdvanceRequestAvailableResponseReplayTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration ownerConfiguration, RtiConfiguration memberConfiguration)
         throws Exception {
      Federate owner = new Federate(factory.getRtiAmbassador());
      Federate member = new Federate(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-time-advance-request-available-response-replay-" + suffix;
      String ownerName = "java-tck-time-advance-available-owner-" + suffix;
      String memberName = "java-tck-time-advance-available-member-" + suffix;
      boolean created = false;
      try {
         owner.connect(callbackModel, ownerConfiguration);
         owner.rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.join(ownerName, federation);

         member.connect(callbackModel, memberConfiguration);
         member.join(memberName, federation);

         LogicalTimeFactory<?, ?> timeFactory = owner.rti.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(timeImplementation),
            "TimeAdvanceRequestAvailable response replay selected an unexpected logical-time factory");
         LogicalTime<?, ?> initial = owner.rti.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         JavaTckSupport.check(epsilon != null,
            "TimeAdvanceRequestAvailable response replay could not allocate an interval");

         owner.rti.enableTimeRegulation(epsilon);
         owner.awaitRegulationEnabled(callbackModel);
         JavaTckSupport.check(owner.regulationEnabledTimes().size() == 1
               && initial.equals(owner.regulationEnabledTimes().get(0)),
            "TimeAdvanceRequestAvailable response replay did not establish regulation exactly once at the initial time");

         LogicalTime<?, ?> target = advanceBy(initial, epsilon, 4);
         owner.rti.timeAdvanceRequestAvailable(target);
         owner.awaitTimeAdvanceGrant(callbackModel);
         List<LogicalTime<?, ?>> grants = owner.timeAdvanceGrants();
         JavaTckSupport.check(grants.size() == 1 && target.equals(grants.get(0)),
            "TimeAdvanceRequestAvailable response replay lost, duplicated, or changed the requested-time grant");

         LogicalTime<?, ?> queriedTime = owner.rti.queryLogicalTime();
         JavaTckSupport.check(target.equals(queriedTime),
            "Query Logical Time after TimeAdvanceRequestAvailable response replay returned another time");
         owner.settleCallbacks(callbackModel);
         JavaTckSupport.check(owner.timeAdvanceGrants().size() == 1,
            "TimeAdvanceRequestAvailable response replay duplicated the Time Advance Grant");

         member.resign(ResignAction.NO_ACTION);
         owner.resign(ResignAction.NO_ACTION);
         owner.rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         member.resignQuietly(ResignAction.NO_ACTION);
         owner.resignQuietly(ResignAction.NO_ACTION);
         if (created) {
            try { owner.rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         member.disconnectQuietly();
         owner.disconnectQuietly();
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> advanceBy(LogicalTime initial,
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = initial;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return (LogicalTime<?, ?>) result;
   }

   private static FederateAmbassador callbacks(Federate federate) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if (arguments != null && arguments.length == 1) {
               if ("timeRegulationEnabled".equals(name)) {
                  federate.regulationEnabledTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("timeAdvanceGrant".equals(name)) {
                  federate.timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
               }
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK TimeAdvanceRequestAvailable replay callbacks";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static final class Federate {
      private final RTIambassador rti;
      private final List<LogicalTime<?, ?>> regulationEnabledTimes =
         new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new CopyOnWriteArrayList<>();
      private boolean connected;
      private boolean joined;

      private Federate(RTIambassador rti) { this.rti = rti; }

      private void connect(CallbackModel callbackModel,
            RtiConfiguration configuration) throws Exception {
         rti.connect(callbacks(this), callbackModel, configuration);
         connected = true;
      }

      private void join(String name, String federation) throws Exception {
         rti.joinFederationExecution(name, "java-tck-federate", federation);
         joined = true;
      }

      private void awaitRegulationEnabled(CallbackModel callbackModel) throws Exception {
         long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
         while (System.nanoTime() < deadline && regulationEnabledTimes.isEmpty()) {
            serviceCallbacks(callbackModel);
         }
         JavaTckSupport.check(!regulationEnabledTimes.isEmpty(),
            "owner timed out waiting for Time Regulation Enabled");
      }

      private void awaitTimeAdvanceGrant(CallbackModel callbackModel) throws Exception {
         long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
         while (System.nanoTime() < deadline && timeAdvanceGrants.isEmpty()) {
            serviceCallbacks(callbackModel);
         }
         JavaTckSupport.check(!timeAdvanceGrants.isEmpty(),
            "owner timed out waiting for Time Advance Grant after response replay");
      }

      private void settleCallbacks(CallbackModel callbackModel) throws Exception {
         long deadline = System.nanoTime() + CALLBACK_SETTLE_NANOS;
         while (System.nanoTime() < deadline) serviceCallbacks(callbackModel);
      }

      private void serviceCallbacks(CallbackModel callbackModel) throws Exception {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            rti.evokeMultipleCallbacks(0.0, 0.001);
         } else {
            Thread.sleep(1L);
         }
      }

      private List<LogicalTime<?, ?>> regulationEnabledTimes() {
         return List.copyOf(regulationEnabledTimes);
      }

      private List<LogicalTime<?, ?>> timeAdvanceGrants() {
         return List.copyOf(timeAdvanceGrants);
      }

      private void resign(ResignAction action) throws Exception {
         if (joined) {
            rti.resignFederationExecution(action);
            joined = false;
         }
      }

      private void resignQuietly(ResignAction action) {
         try { resign(action); }
         catch (Exception ignored) { joined = false; }
      }

      private void disconnectQuietly() {
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
            connected = false;
         }
      }
   }
}
