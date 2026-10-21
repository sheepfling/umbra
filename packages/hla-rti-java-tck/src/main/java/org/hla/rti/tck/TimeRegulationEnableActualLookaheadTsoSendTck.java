package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InvalidLogicalTime;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Post-enable actual-lookahead floor for ordinary timestamped interactions. */
final class TimeRegulationEnableActualLookaheadTsoSendTck {
   private TimeRegulationEnableActualLookaheadTsoSendTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-regulation-lookahead-send-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         sender.connect(callbacks.proxy(), callbackModel);
         connected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         sender.joinFederationExecution("java-tck-regulation-lookahead-sender", federation);
         joined = true;

         InteractionClassHandle interaction =
            sender.getInteractionClassHandle(interactionClassName);
         ParameterHandle parameter = sender.getParameterHandle(interaction, parameterName);
         sender.publishInteractionClass(interaction);
         sender.changeInteractionOrderType(interaction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         LogicalTime initial = sender.queryLogicalTime();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval requestedLookahead =
            (LogicalTimeInterval) epsilon.add(epsilon);
         LogicalTime belowFloor = (LogicalTime) initial.add(epsilon);
         LogicalTime atFloor = (LogicalTime) initial.add(requestedLookahead);

         sender.enableTimeRegulation(requestedLookahead);
         await(sender, callbackModel,
            () -> callbacks.enabledCount == 1,
            "Time Regulation Enabled for the actual-lookahead send boundary");
         JavaTckSupport.check(initial.equals(callbacks.assignedTime)
               && initial.equals(sender.queryLogicalTime())
               && requestedLookahead.equals(sender.queryLookahead()),
            "Time Regulation did not assign the initial time and requested actual lookahead");

         ParameterHandleValueMap values =
            sender.getParameterHandleValueMapFactory().create(1);
         values.put(parameter, new byte[] {0x31});
         byte[] tag = new byte[] {0x32};
         boolean rejected = false;
         try {
            sender.sendInteraction(interaction, values, tag, belowFloor);
         } catch (InvalidLogicalTime expected) {
            rejected = true;
         }
         JavaTckSupport.check(rejected,
            "timestamped Send Interaction accepted a time below the actual-lookahead floor");
         MessageRetractionReturn accepted =
            sender.sendInteraction(interaction, values, tag, atFloor);
         JavaTckSupport.check(accepted != null && accepted.retractionHandleIsValid
               && accepted.handle != null,
            "timestamped Send Interaction rejected the actual-lookahead floor");
         sender.disableTimeRegulation();
         sender.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         sender.destroyFederationExecution(federation);
         created = false;
         sender.disconnect();
         connected = false;
      } finally {
         if (joined) {
            try { sender.disableTimeRegulation(); } catch (Exception ignored) { }
            try { sender.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static final class Recorder {
      private volatile int enabledCount;
      private volatile LogicalTime<?, ?> assignedTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("timeRegulationEnabled".equals(method.getName())
                     && arguments != null && arguments.length == 1) {
                  assignedTime = (LogicalTime<?, ?>) arguments[0];
                  enabledCount++;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK lookahead callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
