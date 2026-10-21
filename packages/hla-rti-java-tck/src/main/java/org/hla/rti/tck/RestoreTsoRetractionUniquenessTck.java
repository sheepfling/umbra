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
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** A restored queued directed TSO retains its handle; later allocations cannot reuse it. */
final class RestoreTsoRetractionUniquenessTck {
   private RestoreTsoRetractionUniquenessTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel model) throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_EVOKED,
         "TSO retraction-uniqueness restore has native evidence only for evoked callbacks");
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder senderEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      List<RTIambassador> members = List.of(sender, receiver);
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-restore-tso-retraction-uniqueness-" + UUID.randomUUID();
      String saveLabel = "tck-tso-retraction-uniqueness";
      byte[] tag = {'U', 'N', 'I', 'Q'};
      boolean created = false;
      try {
         sender.connect(senderEvents.proxy(), model);
         connected.add(sender);
         receiver.connect(receiverEvents.proxy(), model);
         connected.add(receiver);
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         sender.joinFederationExecution("tso-unique-publisher", "publisher", federation);
         joined.add(sender);
         receiver.joinFederationExecution("tso-unique-subscriber", "subscriber", federation);
         joined.add(receiver);

         ObjectClassHandle senderClass = sender.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle senderAttribute = sender.getAttributeHandle(senderClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         InteractionClassHandle senderInteraction = sender.getInteractionClassHandle(
            interactionClassName);
         InteractionClassHandle receiverInteraction = receiver.getInteractionClassHandle(
            interactionClassName);
         AttributeHandleSet senderAttributes = JavaTckSupport.attributeSet(
            sender, senderAttribute);
         AttributeHandleSet receiverAttributes = JavaTckSupport.attributeSet(
            receiver, receiverAttribute);
         InteractionClassHandleSet senderDirected =
            sender.getInteractionClassHandleSetFactory().create();
         senderDirected.add(senderInteraction);
         InteractionClassHandleSet receiverDirected =
            receiver.getInteractionClassHandleSetFactory().create();
         receiverDirected.add(receiverInteraction);
         sender.publishObjectClassAttributes(senderClass, senderAttributes);
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         sender.publishObjectClassDirectedInteractions(senderClass, senderDirected);
         sender.changeInteractionOrderType(senderInteraction, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassDirectedInteractions(receiverClass, receiverDirected);
         ObjectInstanceHandle target = sender.registerObjectInstance(senderClass);
         await(members, () -> receiverEvents.discoveries.contains(target),
            "directed target discovery");

         receiver.enableTimeConstrained();
         LogicalTimeFactory<?, ?> timeFactory = sender.getTimeFactory();
         LogicalTime<?, ?> initial = sender.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time1 = offset(initial, epsilon, 1);
         LogicalTime<?, ?> time6 = offset(initial, epsilon, 6);
         sender.enableTimeRegulation(interval(epsilon, 5));
         await(members, () -> senderEvents.regulationEnabled == 1,
            "time-regulation enablement");

         ParameterHandleValueMap empty = sender.getParameterHandleValueMapFactory().create(0);
         MessageRetractionReturn saved = sender.sendDirectedInteraction(
            senderInteraction, target, empty, tag.clone(), time6);
         JavaTckSupport.check(valid(saved) && receiverEvents.deliveries.isEmpty(),
            "saved directed passel was not queued before recipient grant");
         receiver.timeAdvanceRequest(time1);
         sender.requestFederationSave(saveLabel);
         await(members, () -> senderEvents.saveLabels.equals(List.of(saveLabel))
               && receiverEvents.saveLabels.equals(List.of(saveLabel)),
            "two-member baseline save initiation");
         sender.federateSaveBegun();
         receiver.federateSaveBegun();
         sender.federateSaveComplete();
         receiver.federateSaveComplete();
         await(members, () -> senderEvents.savedCount == 1
               && receiverEvents.savedCount == 1, "two-member baseline saved");

         MessageRetractionReturn discarded = sender.sendDirectedInteraction(
            senderInteraction, target, empty, tag.clone(), time6);
         JavaTckSupport.check(valid(discarded)
               && !saved.handle.equals(discarded.handle),
            "post-save directed passel reused the saved handle");
         sender.requestFederationRestore(saveLabel);
         await(members, () -> senderEvents.restoreLabels.equals(List.of(saveLabel))
               && receiverEvents.restoreLabels.equals(List.of(saveLabel)),
            "two-member restore initiation");
         sender.federateRestoreComplete();
         receiver.federateRestoreComplete();
         await(members, () -> senderEvents.restoredCount == 1
               && receiverEvents.restoredCount == 1, "two-member federation restored");

         JavaTckSupport.expectFailure(() -> sender.retract(discarded.handle),
            "InvalidMessageRetractionHandle", "discarded post-save designator");
         MessageRetractionReturn fresh = sender.sendDirectedInteraction(
            senderInteraction, target, empty, tag.clone(), time6);
         JavaTckSupport.check(valid(fresh)
               && !fresh.handle.equals(discarded.handle)
               && !fresh.handle.equals(saved.handle),
            "post-restore allocation reused an old retraction designator");
         sender.retract(fresh.handle);
         JavaTckSupport.expectFailure(() -> sender.retract(fresh.handle),
            "MessageCanNoLongerBeRetracted", "fresh retraction terminal state");

         receiver.flushQueueRequest(time6);
         await(members, () -> receiverEvents.deliveries.size() == 1
               && receiverEvents.flushGrants.size() == 1,
            "saved directed passel and flush queue grant");
         Delivery delivered = receiverEvents.deliveries.get(0);
         JavaTckSupport.check(delivered.interaction.equals(receiverInteraction)
               && delivered.object.equals(target) && delivered.parameters.isEmpty()
               && Arrays.equals(delivered.tag, tag) && time6.equals(delivered.time)
               && delivered.sentOrder == OrderType.TIMESTAMP
               && delivered.receivedOrder == OrderType.TIMESTAMP
               && saved.handle.equals(delivered.retraction)
               && receiverEvents.order.equals(List.of("interaction", "flush-grant")),
            "fresh retraction aliased or suppressed the saved directed passel");
         sender.retract(saved.handle);
         await(members, () -> receiverEvents.retractions.size() == 1,
            "saved directed retraction callback");
         JavaTckSupport.check(saved.handle.equals(receiverEvents.retractions.get(0)),
            "saved directed retraction callback returned the wrong handle");
         JavaTckSupport.expectFailure(() -> sender.retract(saved.handle),
            "MessageCanNoLongerBeRetracted", "saved retraction terminal state");

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
         if (created) try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         for (int index = connected.size() - 1; index >= 0; --index) {
            try { connected.get(index).disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static boolean valid(MessageRetractionReturn result) {
      return result != null && result.retractionHandleIsValid && result.handle != null;
   }

   private static void await(List<RTIambassador> members,
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

   private static final class Delivery {
      InteractionClassHandle interaction;
      ObjectInstanceHandle object;
      ParameterHandleValueMap parameters;
      byte[] tag;
      LogicalTime<?, ?> time;
      OrderType sentOrder, receivedOrder;
      MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      final List<Delivery> deliveries = new CopyOnWriteArrayList<>();
      final List<LogicalTime<?, ?>> flushGrants = new CopyOnWriteArrayList<>();
      final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      final List<String> saveLabels = new CopyOnWriteArrayList<>();
      final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      final List<String> order = new CopyOnWriteArrayList<>();
      volatile int regulationEnabled, savedCount, restoredCount;

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name))
                  discoveries.add((ObjectInstanceHandle) args[0]);
               else if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("initiateFederateSave".equals(name)) saveLabels.add((String) args[0]);
               else if ("federationSaved".equals(name)) savedCount++;
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) restoredCount++;
               else if ("receiveDirectedInteraction".equals(name) && args.length == 10) {
                  Delivery delivered = new Delivery();
                  delivered.interaction = (InteractionClassHandle) args[0];
                  delivered.object = (ObjectInstanceHandle) args[1];
                  delivered.parameters = ((ParameterHandleValueMap) args[2]).clone();
                  delivered.tag = ((byte[]) args[3]).clone();
                  delivered.time = (LogicalTime<?, ?>) args[6];
                  delivered.sentOrder = (OrderType) args[7];
                  delivered.receivedOrder = (OrderType) args[8];
                  delivered.retraction = (MessageRetractionHandle) args[9];
                  deliveries.add(delivered);
                  order.add("interaction");
               } else if ("flushQueueGrant".equals(name)) {
                  flushGrants.add((LogicalTime<?, ?>) args[0]);
                  order.add("flush-grant");
               } else if ("requestRetraction".equals(name))
                  retractions.add((MessageRetractionHandle) args[0]);
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK TSO uniqueness callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
