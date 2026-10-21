package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
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
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Save and restore a terminal timestamped directed-interaction designator. */
final class RestoreTerminalTsoDirectedInteractionTck {
   private RestoreTerminalTsoDirectedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "terminal directed-interaction restore has native evidence only for evoked callbacks");
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder senderEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      List<RTIambassador> members = List.of(sender, receiver);
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-restore-terminal-tso-directed-" + UUID.randomUUID();
      String saveLabel = "tck-terminal-directed-retraction-baseline";
      byte[] tag = {'T', 'D', '-', 'T'};
      boolean created = false;
      try {
         sender.connect(senderEvents.proxy(), callbackModel);
         connected.add(sender);
         receiver.connect(receiverEvents.proxy(), callbackModel);
         connected.add(receiver);
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         sender.joinFederationExecution("terminal-directed-member", federation);
         joined.add(sender);
         receiver.joinFederationExecution("terminal-directed-member", federation);
         joined.add(receiver);

         ObjectClassHandle senderClass = sender.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle senderAttribute = sender.getAttributeHandle(senderClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         InteractionClassHandle senderInteraction = sender.getInteractionClassHandle(
            interactionClassName);
         InteractionClassHandle receiverInteraction = receiver.getInteractionClassHandle(
            interactionClassName);
         AttributeHandleSet senderAttributes = sender.getAttributeHandleSetFactory().create();
         senderAttributes.add(senderAttribute);
         AttributeHandleSet receiverAttributes = receiver.getAttributeHandleSetFactory().create();
         receiverAttributes.add(receiverAttribute);
         InteractionClassHandleSet senderDirected =
            sender.getInteractionClassHandleSetFactory().create();
         senderDirected.add(senderInteraction);
         InteractionClassHandleSet receiverDirected =
            receiver.getInteractionClassHandleSetFactory().create();
         receiverDirected.add(receiverInteraction);
         sender.publishObjectClassAttributes(senderClass, senderAttributes);
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         sender.publishObjectClassDirectedInteractions(senderClass, senderDirected);
         receiver.subscribeObjectClassDirectedInteractions(receiverClass, receiverDirected);
         ObjectInstanceHandle target = sender.registerObjectInstance(senderClass);
         awaitAll(members, () -> receiverEvents.discoveries.contains(target),
            "terminal directed target discovery");

         receiver.enableTimeConstrained();
         sender.changeInteractionOrderType(senderInteraction, OrderType.TIMESTAMP);
         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "terminal directed members selected different logical-time factories");
         LogicalTime<?, ?> initial = sender.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time1 = offset(initial, epsilon, 1);
         LogicalTime<?, ?> time6 = offset(initial, epsilon, 6);
         sender.enableTimeRegulation(interval(epsilon, 5));
         awaitAll(members, () -> senderEvents.regulationEnabled == 1,
            "terminal directed time-regulation enablement");

         ParameterHandleValueMap empty = sender.getParameterHandleValueMapFactory().create(0);
         MessageRetractionReturn terminal = sender.sendDirectedInteraction(
            senderInteraction, target, empty, tag, time6);
         JavaTckSupport.check(valid(terminal) && receiverEvents.directedInteractions == 0,
            "terminal directed interaction arrived before recipient grant");
         sender.retract(terminal.handle);
         receiver.evokeCallback(0.0);
         JavaTckSupport.check(receiverEvents.directedInteractions == 0,
            "terminal directed interaction was delivered after retraction");
         expectTerminal(sender, terminal.handle);

         MessageRetractionReturn admission = sender.sendDirectedInteraction(
            senderInteraction, target, empty, tag, time6);
         JavaTckSupport.check(valid(admission),
            "directed admission passel lacked a retraction handle");
         receiver.timeAdvanceRequest(time1);
         sender.requestFederationSave(saveLabel);
         awaitAll(members, () -> senderEvents.saveLabels.equals(List.of(saveLabel))
               && receiverEvents.saveLabels.equals(List.of(saveLabel)),
            "terminal directed two-member save initiation");
         sender.federateSaveBegun();
         receiver.federateSaveBegun();
         sender.federateSaveComplete();
         receiver.federateSaveComplete();
         awaitAll(members, () -> senderEvents.savedCount == 1
               && receiverEvents.savedCount == 1,
            "terminal directed federation saved");

         sender.retract(admission.handle);
         receiver.evokeCallback(0.0);
         JavaTckSupport.check(receiverEvents.directedInteractions == 0,
            "post-save admission retraction delivered a directed callback");
         MessageRetractionReturn discarded = sender.sendDirectedInteraction(
            senderInteraction, target, empty, tag, time6);
         JavaTckSupport.check(valid(discarded)
               && !terminal.handle.equals(discarded.handle),
            "post-save directed interaction reused saved terminal designator");
         sender.requestFederationRestore(saveLabel);
         awaitAll(members, () -> senderEvents.restoreRequestSucceeded == 1
               && senderEvents.restoreLabels.equals(List.of(saveLabel))
               && receiverEvents.restoreLabels.equals(List.of(saveLabel)),
            "terminal directed restore initiation");
         sender.federateRestoreComplete();
         receiver.federateRestoreComplete();
         awaitAll(members, () -> senderEvents.restoredCount == 1
               && receiverEvents.restoredCount == 1,
            "terminal directed federation restored");
         expectTerminal(sender, terminal.handle);
         expectInvalid(sender, discarded.handle);

         receiver.unsubscribeObjectClassDirectedInteractions(receiverClass, receiverDirected);
         receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(receiver);
         sender.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(sender);
         sender.destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int index = joined.size() - 1; index >= 0; --index) {
            try { joined.get(index).resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = connected.size() - 1; index >= 0; --index) {
            try { connected.get(index).disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static boolean valid(MessageRetractionReturn result) {
      return result != null && result.retractionHandleIsValid && result.handle != null;
   }

   private static void expectTerminal(RTIambassador member, MessageRetractionHandle handle)
         throws Exception {
      try { member.retract(handle); }
      catch (MessageCanNoLongerBeRetracted expected) { return; }
      throw new IllegalStateException("saved directed handle did not remain terminal");
   }

   private static void expectInvalid(RTIambassador member, MessageRetractionHandle handle)
         throws Exception {
      try { member.retract(handle); }
      catch (InvalidMessageRetractionHandle expected) { return; }
      throw new IllegalStateException("discarded post-save directed handle remained valid");
   }

   private static void awaitAll(List<RTIambassador> members,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         for (RTIambassador member : members) member.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval interval(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int index = 1; index < steps; ++index)
         result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      private volatile int regulationEnabled;
      private volatile int directedInteractions;
      private volatile int savedCount;
      private volatile int restoreRequestSucceeded;
      private volatile int restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && args.length >= 3)
                  discoveries.add((ObjectInstanceHandle) args[0]);
               else if ("receiveDirectedInteraction".equals(name)) ++directedInteractions;
               else if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("initiateFederateSave".equals(name)) saveLabels.add((String) args[0]);
               else if ("federationSaved".equals(name)) ++savedCount;
               else if ("requestFederationRestoreSucceeded".equals(name))
                  ++restoreRequestSucceeded;
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) ++restoredCount;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK terminal directed callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
