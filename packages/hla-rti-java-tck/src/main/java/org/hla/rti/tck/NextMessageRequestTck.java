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
import hla.rti1516_2025.RegionHandleSet;
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

/** Delivers the next timestamped interaction before completing Next Message Request. */
final class NextMessageRequestTck {
   private NextMessageRequestTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-next-message-request-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean publisherRegulating = false;
      boolean receiverConstrained = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-next-message-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-next-message-receiver", federation);
         receiverJoined = true;

         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            "interaction handles did not retain identity across federates");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(
               receiverTimeFactory.getName()),
            "publisher and receiver selected different logical-time factories");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverTimeFactory.makeEpsilon();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "publisher and receiver began at different logical times");

         receiver.enableTimeConstrained();
         receiverConstrained = true;
         publisher.enableTimeRegulation(publisherEpsilon);
         publisherRegulating = true;
         awaitRoles(callbackModel, receiver, receiverRecorder, publisher, publisherRecorder);

         LogicalTime messageTime = add(publisherInitial, offset(publisherEpsilon, 3));
         LogicalTime requestBoundary = add(receiverInitial, offset(receiverEpsilon, 5));
         LogicalTime publisherTarget = add(publisherInitial, offset(publisherEpsilon, 2));
         byte[] value = new byte[] {(byte) 0xc1, (byte) 0xd2};
         byte[] tag = new byte[] {0x4e, 0x4d, 0x52};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, value);
         MessageRetractionReturn sent =
            publisher.sendInteraction(publisherInteraction, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "timestamped Send Interaction returned no valid retraction handle");
         MessageRetractionHandle retraction = sent.handle;
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
            "timestamped interaction arrived before Next Message Request");

         receiver.nextMessageRequest(requestBoundary);
         JavaTckSupport.check(receiverRecorder.timeAdvanceGrantTimes.isEmpty(),
            "Next Message Request completed before the regulator reached the queued message");
         publisher.timeAdvanceRequest(publisherTarget);
         awaitDelivery(callbackModel, publisher, receiver, publisherRecorder, receiverRecorder);

         JavaTckSupport.check(publisherRecorder.timeAdvanceGrantTimes.size() == 1,
            "publisher received an unexpected number of time-advance grants");
         JavaTckSupport.check(receiverRecorder.timeAdvanceGrantTimes.size() == 1
               && receiverRecorder.interactions.size() == 1,
            "receiver received an unexpected number of interaction/grant callbacks");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "Next Message Request delivered its grant before the queued interaction");

         Interaction received = receiverRecorder.interactions.get(0);
         JavaTckSupport.check(received.interaction.equals(receiverInteraction)
               && received.parameters.size() == 1
               && Arrays.equals(value, received.parameters.get(receiverParameter)),
            "Next Message Request delivered the wrong interaction or parameter value");
         JavaTckSupport.check(Arrays.equals(tag, received.tag),
            "Next Message Request did not preserve the interaction tag");
         JavaTckSupport.check(received.transportation != null
               && !receiver.getTransportationTypeName(received.transportation).isEmpty()
               && publisherHandle.equals(received.producer),
            "Next Message Request returned invalid transportation or producer metadata");
         JavaTckSupport.check(messageTime.equals(received.time),
            "Next Message Request returned the wrong logical timestamp");
         JavaTckSupport.check(received.sentOrder == OrderType.TIMESTAMP
               && received.receivedOrder == OrderType.TIMESTAMP
               && retraction.equals(received.retraction),
            "Next Message Request returned incorrect order or retraction metadata");
         JavaTckSupport.check(messageTime.equals(
               receiverRecorder.timeAdvanceGrantTimes.get(0)),
            "Next Message Request did not grant at the next message time");
         JavaTckSupport.check(receiver.queryLogicalTime().equals(messageTime),
            "Next Message Request logical-time query returned the wrong time");
         JavaTckSupport.expectFailure(() -> publisher.retract(retraction),
            "MessageCanNoLongerBeRetracted",
            "retracting the interaction delivered by Next Message Request");

         receiver.disableTimeConstrained();
         receiverConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeInteractionClass(
               receiver.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(
               publisher.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void awaitRoles(CallbackModel callbackModel, RTIambassador receiver,
         Recorder receiverRecorder, RTIambassador publisher, Recorder publisherRecorder)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            receiver.evokeCallback(0.0);
            publisher.evokeCallback(0.0);
         }
         if (receiverRecorder.timeConstrainedEnabled == 1
               && publisherRecorder.timeRegulationEnabled == 1) return;
         Thread.yield();
      }
      throw new AssertionError("time-management role callbacks did not arrive");
   }

   private static void awaitDelivery(CallbackModel callbackModel, RTIambassador publisher,
         RTIambassador receiver, Recorder publisherRecorder, Recorder receiverRecorder)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            publisher.evokeCallback(0.0);
            receiver.evokeCallback(0.0);
         }
         if (publisherRecorder.timeAdvanceGrantTimes.size() >= 1
               && receiverRecorder.timeAdvanceGrantTimes.size() >= 1
               && receiverRecorder.interactions.size() >= 1) return;
         Thread.yield();
      }
      throw new AssertionError("Next Message Request delivery and grants did not arrive");
   }

   private static LogicalTime add(LogicalTime time, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) time.add(interval);
   }

   private static LogicalTimeInterval offset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) {
         result = (LogicalTimeInterval) result.add(epsilon);
      }
      return result;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private RegionHandleSet regions;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes =
         new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Interaction interaction = new Interaction();
                  interaction.interaction = (InteractionClassHandle) arguments[0];
                  interaction.parameters =
                     ((ParameterHandleValueMap) arguments[1]).clone();
                  interaction.tag = ((byte[]) arguments[2]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[3];
                  interaction.producer = (FederateHandle) arguments[4];
                  interaction.regions = (RegionHandleSet) arguments[5];
                  interaction.time = (LogicalTime<?, ?>) arguments[6];
                  interaction.sentOrder = (OrderType) arguments[7];
                  interaction.receivedOrder = (OrderType) arguments[8];
                  interaction.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(interaction);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK Next Message Request callbacks";
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
