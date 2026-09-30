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
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Timestamped interaction retraction behavior when no federate subscribes. */
final class TimestampedInteractionNoFanoutTck {
   private TimestampedInteractionNoFanoutTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-timestamped-interaction-no-fanout-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      boolean published = false;
      try {
         publisher.connect(recorder.proxy(), callbackModel);
         connected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-no-fanout-publisher", federation);
         joined = true;

         InteractionClassHandle interaction =
            publisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle parameter = publisher.getParameterHandle(interaction, parameterName);
         publisher.publishInteractionClass(interaction);
         published = true;
         publisher.changeInteractionOrderType(interaction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeInterval lookahead = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(lookahead);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(recorder.timeRegulationEnabled,
            "timeRegulationEnabled did not cross the publisher callback interface");

         LogicalTime sendTime = offset(publisher.queryLogicalTime(), lookahead, 3);
         MessageRetractionReturn first = send(publisher, interaction, parameter, sendTime);
         JavaTckSupport.check(first != null && first.retractionHandleIsValid && first.handle != null,
            "timestamped Send Interaction returned no valid retraction handle without subscribers");
         JavaTckSupport.drain(publisher);
         checkNoFanout(recorder, "after the first timestamped send");

         publisher.retract(first.handle);
         JavaTckSupport.drain(publisher);
         checkNoFanout(recorder, "after retracting the first timestamped send");
         expectException(() -> publisher.retract(first.handle), "MessageCanNoLongerBeRetracted",
            "retracting the already retracted no-fanout interaction");

         MessageRetractionReturn second = send(publisher, interaction, parameter, sendTime);
         JavaTckSupport.check(second != null && second.retractionHandleIsValid
               && second.handle != null,
            "second timestamped send returned no valid retraction handle without subscribers");
         JavaTckSupport.drain(publisher);
         checkNoFanout(recorder, "after the second timestamped send");

         LogicalTime grantTime = offset(sendTime, lookahead, 1);
         publisher.timeAdvanceRequest(grantTime);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(grantTime.equals(recorder.timeAdvanceGrant)
               && grantTime.equals(publisher.queryLogicalTime()),
            "publisher did not receive the requested time-advance grant");
         checkNoFanout(recorder, "after the time-advance grant");
         expectException(() -> publisher.retract(second.handle), "MessageCanNoLongerBeRetracted",
            "retracting the interaction after its timestamp was passed");
      } finally {
         if (published) {
            try {
               publisher.unpublishInteractionClass(
                  publisher.getInteractionClassHandle(interactionClassName));
            } catch (Exception ignored) { }
         }
         if (joined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static MessageRetractionReturn send(RTIambassador ambassador,
         InteractionClassHandle interaction, ParameterHandle parameter,
         LogicalTime<?, ?> time) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, new byte[] {0x51, 0x52, 0x53});
      return ambassador.sendInteraction(interaction, values, new byte[] {0x61}, time);
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void expectException(CheckedOperation operation, String expected,
         String description) throws Exception {
      try {
         operation.run();
      } catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass()) {
            if (expected.equals(type.getSimpleName())) return;
         }
         throw new AssertionError(description + " threw " + exception.getClass().getName()
            + " instead of " + expected, exception);
      }
      throw new AssertionError(description + " did not throw " + expected);
   }

   private static void checkNoFanout(Recorder recorder, String point) {
      JavaTckSupport.check(recorder.receiveInteractionCallbacks == 0
            && recorder.requestRetractionCallbacks == 0,
         "an interaction or retraction callback was delivered " + point
            + " despite there being no subscribers");
   }

   @FunctionalInterface
   private interface CheckedOperation {
      void run() throws Exception;
   }

   private static final class Recorder {
      private boolean timeRegulationEnabled;
      private int receiveInteractionCallbacks;
      private int requestRetractionCallbacks;
      private LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("receiveInteraction".equals(name)) {
                  ++receiveInteractionCallbacks;
               } else if ("requestRetraction".equals(name)) {
                  ++requestRetractionCallbacks;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK no-fanout callbacks";
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
