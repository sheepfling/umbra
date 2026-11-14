package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandleValueMap;
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

/** Checks that a zero-lookahead NMRA grant permits a TSO send at the grant time. */
final class ZeroLookaheadNmraTsoEqualityAcceptanceTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ZeroLookaheadNmraTsoEqualityAcceptanceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-zero-lookahead-nmra-tso-equality-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean published = false;
      boolean regulating = false;
      InteractionClassHandle interaction = null;
      try {
         sender.connect(callbacks.proxy(), callbackModel);
         connected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         sender.joinFederationExecution("java-tck-zero-lookahead-nmra-sender", federation);
         joined = true;

         interaction = sender.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(interaction != null,
            "Zero-lookahead NMRA interaction lookup returned no handle");
         sender.publishInteractionClass(interaction);
         published = true;
         sender.changeInteractionOrderType(interaction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         LogicalTime<?, ?> initialTime = sender.queryLogicalTime();
         LogicalTimeInterval zeroLookahead =
            (LogicalTimeInterval) timeFactory.makeZero();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         JavaTckSupport.check(zeroLookahead != null,
            "Zero-lookahead NMRA could not allocate a zero interval");
         JavaTckSupport.check(zeroLookahead.isZero(),
            "Logical-time factory returned a nonzero zero-lookahead interval");
         LogicalTime<?, ?> grantTime = offset(initialTime, epsilon, 3);

         sender.enableTimeRegulation(zeroLookahead);
         regulating = true;
         await(sender, callbackModel, () -> callbacks.regulationTimes.size() == 1,
            "zero-lookahead NMRA Time Regulation Enabled callback");

         sender.nextMessageRequestAvailable(grantTime);
         await(sender, callbackModel, () -> callbacks.grants.size() == 1,
            "zero-lookahead NMRA grant");
         JavaTckSupport.check(callbacks.grants.size() == 1
               && grantTime.equals(callbacks.grants.get(0)),
            "zero-lookahead NMRA granted at the wrong logical time");

         ParameterHandleValueMap parameters =
            sender.getParameterHandleValueMapFactory().create(0);
         byte[] tag = new byte[0];
         MessageRetractionReturn send = sender.sendInteraction(
            interaction, parameters, tag, grantTime);
         JavaTckSupport.check(send != null && send.retractionHandleIsValid
               && send.handle != null,
            "Zero-lookahead NMRA rejected a TSO timestamp equal to its grant time");

         sender.disableTimeRegulation();
         regulating = false;
         sender.unpublishInteractionClass(interaction);
         published = false;
         sender.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         sender.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (regulating && joined) {
            try { sender.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (published && joined && interaction != null) {
            try { sender.unpublishInteractionClass(interaction); }
            catch (Exception ignored) { }
         }
         if (joined) resign(sender);
         if (created) {
            try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) disconnect(sender);
      }
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
      private final List<LogicalTime<?, ?>> regulationTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)
                     && arguments != null && arguments.length == 1) {
                  regulationTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK zero-lookahead NMRA callbacks";
                  }
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
