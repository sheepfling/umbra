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
import hla.rti1516_2025.RtiConfiguration;
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

/** Replays an accepted next-message request response while delivering its queued TSO. */
final class ConnectionFaultNextMessageRequestResponseReplayTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long CALLBACK_SETTLE_NANOS = 20_000_000L;

   private ConnectionFaultNextMessageRequestResponseReplayTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String interactionName, String parameterName,
         CallbackModel callbackModel, RtiConfiguration publisherConfiguration,
         RtiConfiguration receiverConfiguration) throws Exception {
      run(factory, fom, mim, timeImplementation, interactionName, parameterName,
         callbackModel, publisherConfiguration, receiverConfiguration, false);
   }

   static void runAvailable(RtiFactory factory, String fom, String mim,
         String timeImplementation, String interactionName, String parameterName,
         CallbackModel callbackModel, RtiConfiguration publisherConfiguration,
         RtiConfiguration receiverConfiguration) throws Exception {
      run(factory, fom, mim, timeImplementation, interactionName, parameterName,
         callbackModel, publisherConfiguration, receiverConfiguration, true);
   }

   private static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String interactionName, String parameterName,
         CallbackModel callbackModel, RtiConfiguration publisherConfiguration,
         RtiConfiguration receiverConfiguration, boolean requestAvailable) throws Exception {
      String requestName = requestAvailable ? "NextMessageRequestAvailable" : "NextMessageRequest";
      String scenarioName = requestAvailable ? "nmra" : "nmr";
      Federate publisher = new Federate(factory.getRtiAmbassador());
      Federate receiver = new Federate(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-" + scenarioName + "-response-replay-" + suffix;
      String publisherName = "java-tck-" + scenarioName + "-publisher-" + suffix;
      String receiverName = "java-tck-" + scenarioName + "-receiver-" + suffix;
      boolean created = false;
      boolean published = false;
      boolean subscribed = false;
      boolean publisherRegulating = false;
      boolean receiverConstrained = false;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      try {
         publisher.connect(callbackModel, publisherConfiguration);
         publisher.rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.join(publisherName, federation);

         receiver.connect(callbackModel, receiverConfiguration);
         receiver.join(receiverName, federation);

         publisherInteraction = publisher.rti.getInteractionClassHandle(interactionName);
         receiverInteraction = receiver.rti.getInteractionClassHandle(interactionName);
         ParameterHandle publisherParameter = publisher.rti.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.rti.getParameterHandle(
            receiverInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction != null && receiverInteraction != null
               && publisherParameter != null && receiverParameter != null
               && publisherInteraction.equals(receiverInteraction)
               && publisherParameter.equals(receiverParameter),
            requestName + " response replay could not resolve consistent interaction handles");

         publisher.rti.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.rti.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.rti.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherFactory = publisher.rti.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.rti.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(timeImplementation)
               && receiverFactory.getName().equals(timeImplementation),
            requestName + " response replay selected an unexpected logical-time factory");
         LogicalTime<?, ?> publisherInitial = publisher.rti.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.rti.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            requestName + " response replay members began at different logical times");
         LogicalTimeInterval<?> publisherEpsilon = publisherFactory.makeEpsilon();
         LogicalTimeInterval<?> receiverEpsilon = receiverFactory.makeEpsilon();
         JavaTckSupport.check(publisherEpsilon != null && receiverEpsilon != null,
            requestName + " response replay could not allocate logical-time intervals");

         receiverConstrained = true;
         receiver.rti.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiver.timeConstrainedEnabledTimes.size() == 1,
            "receiver Time Constrained Enabled callback");
         publisherRegulating = true;
         publisher.rti.enableTimeRegulation(publisherEpsilon);
         await(publisher, callbackModel,
            () -> publisher.timeRegulationEnabledTimes.size() == 1,
            "publisher Time Regulation Enabled callback");

         LogicalTime<?, ?> messageTime = offset(publisherInitial, publisherEpsilon, 3);
         LogicalTime<?, ?> requestBoundary = offset(receiverInitial, receiverEpsilon, 3);
         LogicalTime<?, ?> publisherTarget = offset(publisherInitial, publisherEpsilon, 2);
         JavaTckSupport.check(messageTime.equals(requestBoundary),
            requestName + " response replay boundary does not include the queued TSO exactly at its time");

         byte[] value = new byte[] {0x4e, 0x4d, 0x52};
         byte[] tag = new byte[] {0x52, 0x50, 0x4c};
         ParameterHandleValueMap parameters =
            publisher.rti.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, value.clone());
         MessageRetractionReturn sent = publisher.rti.sendInteraction(
            publisherInteraction, parameters, tag.clone(), messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            requestName + " response replay send returned no valid retraction handle");
         JavaTckSupport.check(receiver.receiveOrderInteractions.isEmpty()
               && receiver.timestampedInteractions.isEmpty()
               && receiver.timeAdvanceGrants.isEmpty(),
            requestName + " response replay delivered queued TSO before the request");

         if (requestAvailable) receiver.rti.nextMessageRequestAvailable(requestBoundary);
         else receiver.rti.nextMessageRequest(requestBoundary);
         publisher.rti.timeAdvanceRequest(publisherTarget);
         awaitDelivery(requestName, callbackModel, publisher, receiver);

         List<LogicalTime<?, ?>> publisherGrants = publisher.timeAdvanceGrants();
         List<LogicalTime<?, ?>> receiverGrants = receiver.timeAdvanceGrants();
         List<Interaction> interactions = receiver.timestampedInteractions();
         JavaTckSupport.check(publisherGrants.size() == 1 && receiverGrants.size() == 1
               && receiver.receiveOrderInteractions.isEmpty() && interactions.size() == 1,
            requestName + " response replay lost or duplicated a grant or TSO callback");
         JavaTckSupport.check(receiver.callbackOrder.equals(Arrays.asList("interaction", "grant")),
            requestName + " response replay callback order was "
               + receiver.callbackOrder + " instead of [interaction, grant]");

         Interaction received = interactions.get(0);
         JavaTckSupport.check(receiverInteraction.equals(received.interaction)
               && received.parameters.size() == 1
               && received.parameters.containsKey(receiverParameter)
               && Arrays.equals(value, received.parameters.get(receiverParameter))
               && Arrays.equals(tag, received.tag)
               && publisherHandle.equals(received.producer)
               && messageTime.equals(received.time)
               && received.sentOrder == OrderType.TIMESTAMP
               && received.receivedOrder == OrderType.TIMESTAMP
               && sent.handle.equals(received.retraction),
            requestName + " response replay changed the queued TSO callback");
         JavaTckSupport.check(publisherTarget.equals(publisherGrants.get(0))
               && messageTime.equals(receiverGrants.get(0)),
            requestName + " response replay granted at the wrong logical time");

         JavaTckSupport.check(messageTime.equals(receiver.rti.queryLogicalTime()),
            "Query Logical Time after " + requestName + " response replay returned another time");
         settleCallbacks(receiver.rti, callbackModel);
         JavaTckSupport.check(receiver.timeAdvanceGrants().size() == 1
               && receiver.timestampedInteractions().size() == 1,
            requestName + " response replay duplicated callbacks after pending events were serviced");

         receiver.rti.disableTimeConstrained();
         receiverConstrained = false;
         publisher.rti.disableTimeRegulation();
         publisherRegulating = false;
         receiver.rti.unsubscribeInteractionClass(receiverInteraction);
         subscribed = false;
         publisher.rti.unpublishInteractionClass(publisherInteraction);
         published = false;
         receiver.resign(ResignAction.NO_ACTION);
         publisher.resign(ResignAction.NO_ACTION);
         publisher.rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverConstrained && receiver.joined) {
            try { receiver.rti.disableTimeConstrained(); }
            catch (Exception ignored) { }
         }
         if (publisherRegulating && publisher.joined) {
            try { publisher.rti.disableTimeRegulation(); }
            catch (Exception ignored) { }
         }
         if (subscribed && receiver.joined && receiverInteraction != null) {
            try { receiver.rti.unsubscribeInteractionClass(receiverInteraction); }
            catch (Exception ignored) { }
         }
         if (published && publisher.joined && publisherInteraction != null) {
            try { publisher.rti.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         receiver.resignQuietly(ResignAction.NO_ACTION);
         publisher.resignQuietly(ResignAction.NO_ACTION);
         if (created) {
            try { publisher.rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         receiver.disconnectQuietly();
         publisher.disconnectQuietly();
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return (LogicalTime<?, ?>) result;
   }

   private static void await(Federate federate, CallbackModel callbackModel,
         BooleanSupplier ready, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !ready.getAsBoolean()) {
         serviceCallbacks(federate.rti, callbackModel);
      }
      JavaTckSupport.check(ready.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitDelivery(String requestName, CallbackModel callbackModel,
         Federate publisher, Federate receiver) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline) {
         serviceCallbacks(publisher.rti, callbackModel);
         serviceCallbacks(receiver.rti, callbackModel);
         if (publisher.timeAdvanceGrants.size() >= 1
               && receiver.timeAdvanceGrants.size() >= 1
               && receiver.timestampedInteractions.size() >= 1) return;
      }
      throw new AssertionError(
         "timed out waiting for " + requestName + " response replay delivery and grant");
   }

   private static void serviceCallbacks(RTIambassador rti, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         rti.evokeMultipleCallbacks(0.0, 0.001);
      } else {
         Thread.sleep(1L);
      }
   }

   private static void settleCallbacks(RTIambassador rti, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_SETTLE_NANOS;
      while (System.nanoTime() < deadline) serviceCallbacks(rti, callbackModel);
   }

   private static FederateAmbassador callbacks(Federate federate) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK next-message request replay callbacks";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            synchronized (federate.callbackLock) {
               if (arguments != null && arguments.length == 1) {
                  if ("timeRegulationEnabled".equals(name)) {
                     federate.timeRegulationEnabledTimes.add((LogicalTime<?, ?>) arguments[0]);
                  } else if ("timeConstrainedEnabled".equals(name)) {
                     federate.timeConstrainedEnabledTimes.add((LogicalTime<?, ?>) arguments[0]);
                  } else if ("timeAdvanceGrant".equals(name)) {
                     federate.callbackOrder.add("grant");
                     federate.timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
                  }
               } else if ("receiveInteraction".equals(name) && arguments != null) {
                  federate.callbackOrder.add("interaction");
                  Interaction interaction = new Interaction(arguments);
                  if (interaction.timestamped) federate.timestampedInteractions.add(interaction);
                  else federate.receiveOrderInteractions.add(interaction);
               }
            }
            return null;
         });
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;
      private final boolean timestamped;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         timestamped = arguments.length == 10;
         if (timestamped) {
            time = (LogicalTime<?, ?>) arguments[6];
            sentOrder = (OrderType) arguments[7];
            receivedOrder = (OrderType) arguments[8];
            retraction = (MessageRetractionHandle) arguments[9];
         } else {
            time = null;
            sentOrder = null;
            receivedOrder = (OrderType) arguments[5];
            retraction = null;
         }
      }
   }

   private static final class Federate {
      private final RTIambassador rti;
      private final List<LogicalTime<?, ?>> timeRegulationEnabledTimes =
         new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeConstrainedEnabledTimes =
         new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new CopyOnWriteArrayList<>();
      private final List<Interaction> receiveOrderInteractions = new CopyOnWriteArrayList<>();
      private final List<Interaction> timestampedInteractions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final Object callbackLock = new Object();
      private boolean connected;
      private boolean joined;

      private Federate(RTIambassador rti) { this.rti = rti; }

      private void connect(CallbackModel callbackModel,
            RtiConfiguration configuration) throws Exception {
         rti.connect(callbacks(this), callbackModel, configuration);
         connected = true;
      }

      private FederateHandle join(String name, String federation) throws Exception {
         FederateHandle handle = rti.joinFederationExecution(name, "java-tck-federate", federation);
         joined = true;
         return handle;
      }

      private List<LogicalTime<?, ?>> timeAdvanceGrants() {
         return List.copyOf(timeAdvanceGrants);
      }

      private List<Interaction> timestampedInteractions() {
         return List.copyOf(timestampedInteractions);
      }

      private void resign(ResignAction action) throws Exception {
         if (joined) {
            rti.resignFederationExecution(action);
            joined = false;
         }
      }

      private void resignQuietly(ResignAction action) {
         try { resign(action); }
         catch (Exception ignored) { joined = false; }
      }

      private void disconnectQuietly() {
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
            connected = false;
         }
      }
   }
}
