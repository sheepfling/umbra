package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Restore the saved live TSO ledger after terminalizing its handle post-save. */
final class RestoreLiveTsoRetractionRecordTck {
   private RestoreLiveTsoRetractionRecordTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel model)
         throws Exception {
      JavaTckSupport.check(model == CallbackModel.HLA_EVOKED,
         "live TSO retraction-record restore has native evidence only for evoked callbacks");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherEvents = new Recorder();
      Recorder receiverEvents = new Recorder();
      List<RTIambassador> members = List.of(publisher, receiver);
      String federation = "java-tck-restore-live-tso-retraction-" + UUID.randomUUID();
      String saveLabel = "tck-live-tso-retraction-baseline";
      byte[] payload = {'Z', 'R'};
      byte[] tag = {'S', 'N', 'P'};
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
            "live-tso-retraction-publisher", "publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("live-tso-retraction-receiver", "subscriber",
            federation);
         receiverJoined = true;
         InteractionClassHandle publisherInteraction = publisher.getInteractionClassHandle(
            interactionClassName);
         InteractionClassHandle receiverInteraction = receiver.getInteractionClassHandle(
            interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload.clone());
         publisher.publishInteractionClass(publisherInteraction);
         receiver.subscribeInteractionClass(receiverInteraction);
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.enableTimeConstrained();
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time1 = offset(initial, epsilon, 1);
         LogicalTime<?, ?> time6 = offset(initial, epsilon, 6);
         publisher.enableTimeRegulation(interval(epsilon, 5));
         await(members, () -> publisherEvents.regulationEnabled == 1,
            "publisher time-regulation enablement");

         MessageRetractionReturn sent = publisher.sendInteraction(
            publisherInteraction, values, tag.clone(), time6);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null && receiverEvents.interactions.isEmpty(),
            "timestamp-six interaction was not queued before save");
         receiver.timeAdvanceRequest(time1);
         publisher.requestFederationSave(saveLabel);
         await(members, () -> publisherEvents.saveLabels.equals(List.of(saveLabel))
               && receiverEvents.saveLabels.equals(List.of(saveLabel)),
            "live retraction two-member save initiation");
         publisher.federateSaveBegun();
         receiver.federateSaveBegun();
         publisher.federateSaveComplete();
         receiver.federateSaveComplete();
         await(members, () -> publisherEvents.savedCount == 1
               && receiverEvents.savedCount == 1, "live retraction baseline saved");

         publisher.retract(sent.handle);
         receiver.evokeCallback(0.0);
         JavaTckSupport.check(receiverEvents.interactions.isEmpty()
               && receiverEvents.retractions.isEmpty(),
            "post-save retraction leaked a queued callback");
         JavaTckSupport.expectFailure(() -> publisher.retract(sent.handle),
            "MessageCanNoLongerBeRetracted", "post-save terminal designator");
         publisher.requestFederationRestore(saveLabel);
         await(members, () -> publisherEvents.restoreLabels.equals(List.of(saveLabel))
               && receiverEvents.restoreLabels.equals(List.of(saveLabel)),
            "live retraction two-member restore initiation");
         publisher.federateRestoreComplete();
         receiver.federateRestoreComplete();
         await(members, () -> publisherEvents.restoredCount == 1
               && receiverEvents.restoredCount == 1,
            "live retraction federation restored");
         JavaTckSupport.check(receiverEvents.interactions.isEmpty(),
            "restored interaction arrived before recipient FQR");

         receiverEvents.order.clear();
         receiver.flushQueueRequest(time6);
         await(members, () -> receiverEvents.interactions.size() == 1
               && receiverEvents.flushGrants.size() == 1,
            "restored live interaction and flush queue grant");
         Delivery delivered = receiverEvents.interactions.get(0);
         JavaTckSupport.check(delivered.interaction.equals(receiverInteraction)
               && delivered.parameters.size() == 1
               && Arrays.equals(delivered.parameters.get(receiverParameter), payload)
               && Arrays.equals(delivered.tag, tag)
               && delivered.producer.equals(publisherHandle)
               && delivered.time.equals(time6)
               && delivered.sentOrder == OrderType.TIMESTAMP
               && delivered.receivedOrder == OrderType.TIMESTAMP
               && delivered.retraction.equals(sent.handle)
               && receiverEvents.order.equals(List.of("interaction", "flush-grant")),
            "restored live interaction changed payload, designator, or callback ordering");
         publisher.retract(sent.handle);
         await(members, () -> receiverEvents.retractions.size() == 1,
            "restored live request-retraction callback");
         JavaTckSupport.check(receiverEvents.retractions.get(0).equals(sent.handle),
            "restored live retraction callback returned a different designator");
         JavaTckSupport.expectFailure(() -> publisher.retract(sent.handle),
            "MessageCanNoLongerBeRetracted", "restored terminal designator");

         receiver.disableTimeConstrained();
         publisher.disableTimeRegulation();
         receiver.unsubscribeInteractionClass(receiverInteraction);
         publisher.unpublishInteractionClass(publisherInteraction);
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
      ParameterHandleValueMap parameters;
      byte[] tag;
      FederateHandle producer;
      LogicalTime<?, ?> time;
      OrderType sentOrder, receivedOrder;
      MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      final List<String> saveLabels = new CopyOnWriteArrayList<>();
      final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      final List<Delivery> interactions = new CopyOnWriteArrayList<>();
      final List<LogicalTime<?, ?>> flushGrants = new CopyOnWriteArrayList<>();
      final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      final List<String> order = new CopyOnWriteArrayList<>();
      volatile int regulationEnabled, savedCount, restoredCount;

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) regulationEnabled++;
               else if ("initiateFederateSave".equals(name)) saveLabels.add((String) args[0]);
               else if ("federationSaved".equals(name)) savedCount++;
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) restoredCount++;
               else if ("receiveInteraction".equals(name) && args.length == 10) {
                  Delivery delivered = new Delivery();
                  delivered.interaction = (InteractionClassHandle) args[0];
                  delivered.parameters = ((ParameterHandleValueMap) args[1]).clone();
                  delivered.tag = ((byte[]) args[2]).clone();
                  delivered.producer = (FederateHandle) args[4];
                  delivered.time = (LogicalTime<?, ?>) args[6];
                  delivered.sentOrder = (OrderType) args[7];
                  delivered.receivedOrder = (OrderType) args[8];
                  delivered.retraction = (MessageRetractionHandle) args[9];
                  interactions.add(delivered);
                  order.add("interaction");
               } else if ("flushQueueGrant".equals(name)) {
                  flushGrants.add((LogicalTime<?, ?>) args[0]);
                  order.add("flush-grant");
               } else if ("requestRetraction".equals(name))
                  retractions.add((MessageRetractionHandle) args[0]);
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK live TSO retraction callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
