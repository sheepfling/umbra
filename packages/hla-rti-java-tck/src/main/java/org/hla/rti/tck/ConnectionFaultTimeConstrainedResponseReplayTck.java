package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Replays an accepted EnableTimeConstrained response after an adapter-managed fault. */
final class ConnectionFaultTimeConstrainedResponseReplayTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long CALLBACK_SETTLE_NANOS = 20_000_000L;

   private ConnectionFaultTimeConstrainedResponseReplayTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration ownerConfiguration, RtiConfiguration memberConfiguration)
         throws Exception {
      Federate owner = new Federate(factory.getRtiAmbassador());
      Federate member = new Federate(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-time-constrained-response-replay-" + suffix;
      String ownerName = "java-tck-time-constrained-owner-" + suffix;
      String memberName = "java-tck-time-constrained-member-" + suffix;
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
            "EnableTimeConstrained response replay selected an unexpected logical-time factory");
         LogicalTime<?, ?> initial = owner.rti.queryLogicalTime();

         owner.rti.enableTimeConstrained();
         owner.awaitTimeConstrainedEnabled(callbackModel);
         List<LogicalTime<?, ?>> enabledTimes = owner.timeConstrainedEnabledTimes();
         JavaTckSupport.check(enabledTimes.size() == 1 && initial.equals(enabledTimes.get(0)),
            "EnableTimeConstrained response replay lost, duplicated, or changed the callback time");

         LogicalTime<?, ?> queriedTime = owner.rti.queryLogicalTime();
         JavaTckSupport.check(initial.equals(queriedTime),
            "Query Logical Time after EnableTimeConstrained response replay returned another time");
         owner.settleCallbacks(callbackModel);
         JavaTckSupport.check(owner.timeConstrainedEnabledTimes().size() == 1,
            "EnableTimeConstrained response replay duplicated the callback after a time service");

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

   private static FederateAmbassador callbacks(Federate federate) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if ("timeConstrainedEnabled".equals(name)
                  && arguments != null && arguments.length == 1) {
               federate.timeConstrainedEnabledTimes.add((LogicalTime<?, ?>) arguments[0]);
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK time-constrained replay callbacks";
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
      private final List<LogicalTime<?, ?>> timeConstrainedEnabledTimes =
         new CopyOnWriteArrayList<>();
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

      private void awaitTimeConstrainedEnabled(CallbackModel callbackModel) throws Exception {
         long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
         while (System.nanoTime() < deadline && timeConstrainedEnabledTimes.isEmpty()) {
            serviceCallbacks(callbackModel);
         }
         JavaTckSupport.check(!timeConstrainedEnabledTimes.isEmpty(),
            "owner timed out waiting for Time Constrained Enabled after response replay");
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

      private List<LogicalTime<?, ?>> timeConstrainedEnabledTimes() {
         return List.copyOf(timeConstrainedEnabledTimes);
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
