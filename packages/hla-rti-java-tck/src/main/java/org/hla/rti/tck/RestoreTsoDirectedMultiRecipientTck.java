package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
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
import hla.rti1516_2025.TransportationTypeHandle;
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

/** Restore one queued directed TSO and independently deliver it to two recipients. */
final class RestoreTsoDirectedMultiRecipientTck {
   private RestoreTsoDirectedMultiRecipientTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel model) throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_EVOKED,
         "multi-recipient directed restore has native evidence only for evoked callbacks");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder firstEvents = new Recorder();
      Recorder secondEvents = new Recorder();
      List<RTIambassador> members = List.of(publisher, first, second);
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-restore-tso-directed-multi-" + UUID.randomUUID();
      String saveLabel = "tck-directed-tso-multi-recipient-restore";
      byte[] tag = {'M', 'R', 'F', '2'};
      boolean created = false;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         connected.add(publisher);
         first.connect(firstEvents.proxy(), model);
         connected.add(first);
         second.connect(secondEvents.proxy(), model);
         connected.add(second);
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "tso-directed-multi-publisher", "publisher", federation);
         joined.add(publisher);
         first.joinFederationExecution("tso-directed-multi-first", "subscriber", federation);
         joined.add(first);
         second.joinFederationExecution("tso-directed-multi-second", "subscriber", federation);
         joined.add(second);

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         InteractionClassHandle publisherInteraction = publisher.getInteractionClassHandle(
            interactionClassName);
         AttributeHandleSet publishedAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         InteractionClassHandleSet publishedDirected =
            publisher.getInteractionClassHandleSetFactory().create();
         publishedDirected.add(publisherInteraction);
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes);
         publisher.publishObjectClassDirectedInteractions(publisherClass, publishedDirected);
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         RecipientHandles firstHandles = subscribe(first, objectClassName,
            attributeName, interactionClassName);
         RecipientHandles secondHandles = subscribe(second, objectClassName,
            attributeName, interactionClassName);
         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         await(members, () -> firstEvents.discoveries.contains(target)
               && secondEvents.discoveries.contains(target),
            "both directed target discoveries");

         first.enableTimeConstrained();
         second.enableTimeConstrained();
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time1 = offset(initial, epsilon, 1);
         LogicalTime<?, ?> time8 = offset(initial, epsilon, 8);
         publisher.enableTimeRegulation(interval(epsilon, 5));
         await(members, () -> publisherEvents.regulationEnabled == 1,
            "publisher time-regulation enablement");

         ParameterHandleValueMap empty = publisher.getParameterHandleValueMapFactory().create(0);
         MessageRetractionReturn saved = publisher.sendDirectedInteraction(
            publisherInteraction, target, empty, tag.clone(), time8);
         JavaTckSupport.check(valid(saved) && firstEvents.deliveries.isEmpty()
               && secondEvents.deliveries.isEmpty(),
            "timestamp-eight directed passel was not queued for both recipients");
         first.timeAdvanceRequest(time1);
         second.timeAdvanceRequest(time1);
         publisher.requestFederationSave(saveLabel);
         await(members, () -> publisherEvents.saveLabels.equals(List.of(saveLabel))
               && firstEvents.saveLabels.equals(List.of(saveLabel))
               && secondEvents.saveLabels.equals(List.of(saveLabel)),
            "three-member save initiation");
         publisher.federateSaveBegun();
         first.federateSaveBegun();
         second.federateSaveBegun();
         publisher.federateSaveComplete();
         first.federateSaveComplete();
         second.federateSaveComplete();
         await(members, () -> publisherEvents.savedCount == 1
               && firstEvents.savedCount == 1 && secondEvents.savedCount == 1,
            "three-member federation saved");

         MessageRetractionReturn discarded = publisher.sendDirectedInteraction(
            publisherInteraction, target, empty, tag.clone(), time8);
         JavaTckSupport.check(valid(discarded)
               && !saved.handle.equals(discarded.handle),
            "post-save directed designator reused the saved handle");
         publisher.retract(discarded.handle);
         first.evokeCallback(0.0);
         second.evokeCallback(0.0);
         JavaTckSupport.check(firstEvents.deliveries.isEmpty()
               && secondEvents.deliveries.isEmpty(),
            "post-save retracted passel reached a recipient");
         JavaTckSupport.expectFailure(() -> publisher.retract(discarded.handle),
            "MessageCanNoLongerBeRetracted", "post-save terminal designator");
         publisher.requestFederationRestore(saveLabel);
         await(members, () -> publisherEvents.restoreLabels.equals(List.of(saveLabel))
               && firstEvents.restoreLabels.equals(List.of(saveLabel))
               && secondEvents.restoreLabels.equals(List.of(saveLabel)),
            "three-member restore initiation");
         publisher.federateRestoreComplete();
         first.federateRestoreComplete();
         second.federateRestoreComplete();
         await(members, () -> publisherEvents.restoredCount == 1
               && firstEvents.restoredCount == 1 && secondEvents.restoredCount == 1,
            "three-member federation restored");
         JavaTckSupport.check(firstEvents.deliveries.isEmpty()
               && secondEvents.deliveries.isEmpty(),
            "restored directed passel arrived before recipient FQR");
         JavaTckSupport.expectFailure(() -> publisher.retract(discarded.handle),
            "InvalidMessageRetractionHandle", "discarded post-save designator");

         firstEvents.order.clear();
         secondEvents.order.clear();
         first.flushQueueRequest(time8);
         await(members, () -> firstEvents.deliveries.size() == 1
               && firstEvents.flushGrants.size() == 1,
            "first restored directed recipient FQR");
         JavaTckSupport.check(secondEvents.deliveries.isEmpty(),
            "second recipient was released by the first FQR");
         second.flushQueueRequest(time8);
         await(members, () -> secondEvents.deliveries.size() == 1
               && secondEvents.flushGrants.size() == 1,
            "second restored directed recipient FQR");
         TransportationTypeHandle reliable = publisher.getTransportationTypeHandle(
            "HLAreliable");
         checkDelivery(firstEvents, firstHandles, target, publisherHandle,
            reliable, saved.handle, time8, tag);
         checkDelivery(secondEvents, secondHandles, target, publisherHandle,
            reliable, saved.handle, time8, tag);
         publisher.retract(saved.handle);
         await(members, () -> firstEvents.retractions.size() == 1
               && secondEvents.retractions.size() == 1,
            "both restored directed request-retraction callbacks");
         JavaTckSupport.check(saved.handle.equals(firstEvents.retractions.get(0))
               && saved.handle.equals(secondEvents.retractions.get(0)),
            "recipient retraction callbacks returned a different designator");
         JavaTckSupport.expectFailure(() -> publisher.retract(saved.handle),
            "MessageCanNoLongerBeRetracted", "saved terminal designator");

         unsubscribe(first, firstHandles);
         unsubscribe(second, secondHandles);
         first.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(first);
         second.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(second);
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(publisher);
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int index = joined.size() - 1; index >= 0; --index) {
            try { joined.get(index).resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         for (int index = connected.size() - 1; index >= 0; --index) {
            try { connected.get(index).disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static RecipientHandles subscribe(RTIambassador recipient,
         String objectClassName, String attributeName, String interactionClassName)
         throws Exception {
      RecipientHandles handles = new RecipientHandles();
      handles.objectClass = recipient.getObjectClassHandle(objectClassName);
      handles.attribute = recipient.getAttributeHandle(handles.objectClass, attributeName);
      handles.interaction = recipient.getInteractionClassHandle(interactionClassName);
      handles.attributes = JavaTckSupport.attributeSet(recipient, handles.attribute);
      handles.directed = recipient.getInteractionClassHandleSetFactory().create();
      handles.directed.add(handles.interaction);
      recipient.subscribeObjectClassAttributes(handles.objectClass, handles.attributes);
      recipient.subscribeObjectClassDirectedInteractions(handles.objectClass, handles.directed);
      return handles;
   }

   private static void unsubscribe(RTIambassador recipient, RecipientHandles handles)
         throws Exception {
      recipient.unsubscribeObjectClassDirectedInteractions(
         handles.objectClass, handles.directed);
      recipient.unsubscribeObjectClassAttributes(handles.objectClass, handles.attributes);
   }

   private static void checkDelivery(Recorder events, RecipientHandles handles,
         ObjectInstanceHandle target, FederateHandle publisher,
         TransportationTypeHandle reliable, MessageRetractionHandle saved,
         LogicalTime<?, ?> time8, byte[] tag) {
      Delivery delivered = events.deliveries.get(0);
      JavaTckSupport.check(delivered.interaction.equals(handles.interaction)
            && delivered.object.equals(target) && delivered.parameters.isEmpty()
            && Arrays.equals(delivered.tag, tag)
            && delivered.transportation.equals(reliable)
            && delivered.producer.equals(publisher)
            && delivered.time.equals(time8)
            && delivered.sentOrder == OrderType.TIMESTAMP
            && delivered.receivedOrder == OrderType.TIMESTAMP
            && delivered.retraction.equals(saved)
            && events.order.equals(List.of("directed", "flush-grant")),
         "restored directed callback changed public fields or ordering");
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

   private static final class RecipientHandles {
      ObjectClassHandle objectClass;
      AttributeHandle attribute;
      InteractionClassHandle interaction;
      AttributeHandleSet attributes;
      InteractionClassHandleSet directed;
   }

   private static final class Delivery {
      InteractionClassHandle interaction;
      ObjectInstanceHandle object;
      ParameterHandleValueMap parameters;
      byte[] tag;
      TransportationTypeHandle transportation;
      FederateHandle producer;
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
                  delivered.transportation = (TransportationTypeHandle) args[4];
                  delivered.producer = (FederateHandle) args[5];
                  delivered.time = (LogicalTime<?, ?>) args[6];
                  delivered.sentOrder = (OrderType) args[7];
                  delivered.receivedOrder = (OrderType) args[8];
                  delivered.retraction = (MessageRetractionHandle) args[9];
                  deliveries.add(delivered);
                  order.add("directed");
               } else if ("flushQueueGrant".equals(name)) {
                  flushGrants.add((LogicalTime<?, ?>) args[0]);
                  order.add("flush-grant");
               } else if ("requestRetraction".equals(name))
                  retractions.add((MessageRetractionHandle) args[0]);
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed multi-restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
