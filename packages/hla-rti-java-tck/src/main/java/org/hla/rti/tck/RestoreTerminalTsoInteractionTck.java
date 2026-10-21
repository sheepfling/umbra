package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InvalidMessageRetractionHandle;
import hla.rti1516_2025.exceptions.MessageCanNoLongerBeRetracted;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Saved terminal timestamped interaction-retraction classification through the public API. */
final class RestoreTerminalTsoInteractionTck {
   private RestoreTerminalTsoInteractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "terminal timestamped interaction restore has native evidence only for evoked callbacks");
      RTIambassador sender = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-restore-terminal-tso-interaction-" + UUID.randomUUID();
      String saveLabel = "tck-terminal-tso-retraction-baseline";
      byte[] payload = {'T', 'M'};
      byte[] tag = {'T', 'O', 'M'};
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         sender.connect(callbacks.proxy(), callbackModel);
         connected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         sender.joinFederationExecution("terminal-interaction-publisher", federation);
         joined = true;
         InteractionClassHandle interaction = sender.getInteractionClassHandle(
            interactionClassName);
         ParameterHandle parameter = sender.getParameterHandle(interaction, parameterName);
         ParameterHandleValueMap parameters = sender.getParameterHandleValueMapFactory().create(1);
         parameters.put(parameter, payload);
         sender.publishInteractionClass(interaction);
         sender.changeInteractionOrderType(interaction, OrderType.TIMESTAMP);
         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         LogicalTime<?, ?> initial = sender.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> sendTime = offset(initial, epsilon, 6);
         sender.enableTimeRegulation(epsilon);
         await(sender, () -> callbacks.regulationEnabled == 1,
            "terminal TSO interaction time-regulation enablement");

         MessageRetractionReturn terminal = sender.sendInteraction(
            interaction, parameters, tag, sendTime);
         JavaTckSupport.check(valid(terminal),
            "terminal TSO interaction lacked a retraction handle");
         sender.retract(terminal.handle);
         expectTerminal(sender, terminal.handle);

         sender.requestFederationSave(saveLabel);
         await(sender, () -> callbacks.saveLabels.equals(List.of(saveLabel)),
            "terminal TSO interaction save initiation");
         sender.federateSaveBegun();
         sender.federateSaveComplete();
         await(sender, () -> callbacks.savedCount == 1,
            "terminal TSO interaction federation saved");

         MessageRetractionReturn discarded = sender.sendInteraction(
            interaction, parameters, tag, sendTime);
         JavaTckSupport.check(valid(discarded)
               && !terminal.handle.equals(discarded.handle),
            "post-save interaction reused the terminal designator");
         sender.requestFederationRestore(saveLabel);
         await(sender, () -> callbacks.restoreRequestSucceeded == 1
               && callbacks.restoreLabels.equals(List.of(saveLabel)),
            "terminal TSO interaction restore initiation");
         sender.federateRestoreComplete();
         await(sender, () -> callbacks.restoredCount == 1,
            "terminal TSO interaction federation restored");
         expectTerminal(sender, terminal.handle);
         expectInvalid(sender, discarded.handle);

         sender.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         sender.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) {
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

   private static boolean valid(MessageRetractionReturn value) {
      return value != null && value.retractionHandleIsValid && value.handle != null;
   }

   private static void expectTerminal(RTIambassador member, MessageRetractionHandle handle)
         throws Exception {
      try { member.retract(handle); }
      catch (MessageCanNoLongerBeRetracted expected) { return; }
      throw new IllegalStateException("saved interaction handle did not remain terminal");
   }

   private static void expectInvalid(RTIambassador member, MessageRetractionHandle handle)
         throws Exception {
      try { member.retract(handle); }
      catch (InvalidMessageRetractionHandle expected) { return; }
      throw new IllegalStateException("discarded post-save interaction handle remained valid");
   }

   private static void await(RTIambassador member, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline)
         member.evokeCallback(0.0);
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static final class Recorder {
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      private volatile int regulationEnabled;
      private volatile int savedCount;
      private volatile int restoreRequestSucceeded;
      private volatile int restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("initiateFederateSave".equals(name)) saveLabels.add((String) args[0]);
               else if ("federationSaved".equals(name)) ++savedCount;
               else if ("requestFederationRestoreSucceeded".equals(name))
                  ++restoreRequestSucceeded;
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) ++restoredCount;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK terminal TSO interaction callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
