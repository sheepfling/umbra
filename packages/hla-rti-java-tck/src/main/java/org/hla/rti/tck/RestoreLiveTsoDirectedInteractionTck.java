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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Restore a saved live directed TSO after terminalizing the same handle post-save. */
final class RestoreLiveTsoDirectedInteractionTck {
   private RestoreLiveTsoDirectedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel model) throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_EVOKED,
         "live directed restore has native evidence only for evoked callbacks");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      List<RTIambassador> members = List.of(publisher, receiver);
      String federation = "java-tck-restore-live-tso-directed-" + UUID.randomUUID();
      String saveLabel = "tck-live-directed-retraction-baseline";
      byte[] tag = {'S', 'D', 'I', '6'};
      boolean publisherConnected = false, receiverConnected = false;
      boolean created = false, publisherJoined = false, receiverJoined = false;
      try {
         publisher.connect(publisherEvents.proxy(), model);
         publisherConnected = true;
         receiver.connect(receiverEvents.proxy(), model);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "live-directed-publisher", "publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("live-directed-receiver", "subscriber", federation);
         receiverJoined = true;
         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         InteractionClassHandle publisherInteraction = publisher.getInteractionClassHandle(
            interactionClassName);
         InteractionClassHandle receiverInteraction = receiver.getInteractionClassHandle(
            interactionClassName);
         AttributeHandleSet publishedAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         AttributeHandleSet subscribedAttributes = JavaTckSupport.attributeSet(
            receiver, receiverAttribute);
         InteractionClassHandleSet publishedDirected =
            publisher.getInteractionClassHandleSetFactory().create();
         publishedDirected.add(publisherInteraction);
         InteractionClassHandleSet subscribedDirected =
            receiver.getInteractionClassHandleSetFactory().create();
         subscribedDirected.add(receiverInteraction);
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes);
         receiver.subscribeObjectClassAttributes(receiverClass, subscribedAttributes);
         publisher.publishObjectClassDirectedInteractions(publisherClass, publishedDirected);
         receiver.subscribeObjectClassDirectedInteractions(receiverClass, subscribedDirected);
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         await(members, () -> receiverEvents.discoveries.contains(target),
            "directed target discovery");
         receiver.enableTimeConstrained();
         receiver.evokeCallback(0.0);
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time1 = offset(initial, epsilon, 1);
         LogicalTime<?, ?> time6 = offset(initial, epsilon, 6);
         publisher.enableTimeRegulation(interval(epsilon, 5));
         await(members, () -> publisherEvents.regulationEnabled == 1,
            "publisher time-regulation enablement");

         ParameterHandleValueMap empty = publisher.getParameterHandleValueMapFactory().create(0);
         MessageRetractionReturn saved = publisher.sendDirectedInteraction(
            publisherInteraction, target, empty, tag.clone(), time6);
         JavaTckSupport.check(saved != null && saved.retractionHandleIsValid
               && saved.handle != null && receiverEvents.deliveries.isEmpty(),
            "timestamp-six directed interaction was not queued before save");
         receiver.timeAdvanceRequest(time1);
         publisher.requestFederationSave(saveLabel);
         await(members, () -> publisherEvents.saveLabels.equals(List.of(saveLabel))
               && receiverEvents.saveLabels.equals(List.of(saveLabel)),
            "two-member directed save initiation");
         publisher.federateSaveBegun();
         receiver.federateSaveBegun();
         publisher.federateSaveComplete();
         receiver.federateSaveComplete();
         await(members, () -> publisherEvents.savedCount == 1
               && receiverEvents.savedCount == 1, "directed federation saved");

         publisher.retract(saved.handle);
         receiver.evokeCallback(0.0);
         JavaTckSupport.check(receiverEvents.deliveries.isEmpty()
               && receiverEvents.retractions.isEmpty(),
            "post-save retraction leaked a directed callback");
         JavaTckSupport.expectFailure(() -> publisher.retract(saved.handle),
            "MessageCanNoLongerBeRetracted", "post-save terminal directed handle");
         publisher.requestFederationRestore(saveLabel);
         await(members, () -> publisherEvents.restoreLabels.equals(List.of(saveLabel))
               && receiverEvents.restoreLabels.equals(List.of(saveLabel)),
            "two-member directed restore initiation");
         publisher.federateRestoreComplete();
         receiver.federateRestoreComplete();
         await(members, () -> publisherEvents.restoredCount == 1
               && receiverEvents.restoredCount == 1, "directed federation restored");
         JavaTckSupport.check(receiverEvents.deliveries.isEmpty(),
            "restored directed interaction arrived before receiver FQR");

         receiverEvents.order.clear();
         receiver.flushQueueRequest(time6);
         await(members, () -> receiverEvents.deliveries.size() == 1
               && receiverEvents.flushGrants.size() == 1,
            "restored directed delivery and flush queue grant");
         Delivery delivered = receiverEvents.deliveries.get(0);
         TransportationTypeHandle reliable = publisher.getTransportationTypeHandle(
            "HLAreliable");
         JavaTckSupport.check(delivered.interaction.equals(receiverInteraction)
               && delivered.object.equals(target) && delivered.parameters.isEmpty()
               && Arrays.equals(delivered.tag, tag)
               && delivered.transportation.equals(reliable)
               && delivered.producer.equals(publisherHandle)
               && delivered.time.equals(time6)
               && delivered.sentOrder == OrderType.TIMESTAMP
               && delivered.receivedOrder == OrderType.TIMESTAMP
               && delivered.retraction.equals(saved.handle)
               && receiverEvents.order.equals(List.of("directed", "flush-grant")),
            "restored directed callback changed public fields or ordering");
         publisher.retract(saved.handle);
         await(members, () -> receiverEvents.retractions.size() == 1,
            "restored directed request-retraction callback");
         JavaTckSupport.check(receiverEvents.retractions.get(0).equals(saved.handle),
            "restored directed retraction callback returned a different handle");
         JavaTckSupport.expectFailure(() -> publisher.retract(saved.handle),
            "MessageCanNoLongerBeRetracted", "restored terminal directed handle");

         receiver.unsubscribeObjectClassDirectedInteractions(receiverClass, subscribedDirected);
         receiver.unsubscribeObjectClassAttributes(receiverClass, subscribedAttributes);
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void await(List<RTIambassador> members,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline)
         for (RTIambassador member : members) member.evokeCallback(0.0);
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
                  if ("toString".equals(name)) return "Java TCK live directed restore callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
