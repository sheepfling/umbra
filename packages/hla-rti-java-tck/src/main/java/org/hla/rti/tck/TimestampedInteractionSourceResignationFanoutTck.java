package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
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
import java.util.function.BooleanSupplier;

/** Fans one queued timestamped interaction out after producer resignation. */
final class TimestampedInteractionSourceResignationFanoutTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedInteractionSourceResignationFanoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      Recorder clockRecorder = new Recorder();
      String federation = "java-tck-interaction-resignation-fanout-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean firstConnected = false;
      boolean secondConnected = false;
      boolean clockConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean firstJoined = false;
      boolean secondJoined = false;
      boolean clockJoined = false;
      boolean published = false;
      boolean firstSubscribed = false;
      boolean secondSubscribed = false;
      boolean publisherRegulating = false;
      boolean firstConstrained = false;
      boolean secondConstrained = false;
      boolean clockRegulating = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         first.connect(firstRecorder.proxy(), callbackModel);
         firstConnected = true;
         second.connect(secondRecorder.proxy(), callbackModel);
         secondConnected = true;
         clock.connect(clockRecorder.proxy(), callbackModel);
         clockConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer =
            publisher.joinFederationExecution("java-tck-resignation-fanout-producer", federation);
         publisherJoined = true;
         first.joinFederationExecution("java-tck-resignation-fanout-first", federation);
         firstJoined = true;
         second.joinFederationExecution("java-tck-resignation-fanout-second", federation);
         secondJoined = true;
         clock.joinFederationExecution("java-tck-resignation-fanout-clock", federation);
         clockJoined = true;

         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle firstInteraction =
            first.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle secondInteraction =
            second.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle firstParameter = first.getParameterHandle(firstInteraction, parameterName);
         ParameterHandle secondParameter =
            second.getParameterHandle(secondInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction != null && firstInteraction != null
               && secondInteraction != null && publisherParameter != null
               && firstParameter != null && secondParameter != null,
            "timestamped interaction fan-out FOM lookup returned a null handle");

         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         first.subscribeInteractionClass(firstInteraction);
         firstSubscribed = true;
         second.subscribeInteractionClass(secondInteraction);
         secondSubscribed = true;

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> firstFactory = first.getTimeFactory();
         LogicalTimeFactory<?, ?> secondFactory = second.getTimeFactory();
         LogicalTimeFactory<?, ?> clockFactory = clock.getTimeFactory();
         String factoryName = publisherFactory.getName();
         JavaTckSupport.check(factoryName.equals(firstFactory.getName())
               && factoryName.equals(secondFactory.getName())
               && factoryName.equals(clockFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval publisherLookahead = intervalOffset(publisherFactory.makeEpsilon(), 5);
         LogicalTimeInterval clockLookahead = intervalOffset(clockFactory.makeEpsilon(), 5);

         first.enableTimeConstrained();
         firstConstrained = true;
         await(first, callbackModel, () -> firstRecorder.timeConstrainedEnabledCount == 1,
            "first recipient Time Constrained enable");
         second.enableTimeConstrained();
         secondConstrained = true;
         await(second, callbackModel, () -> secondRecorder.timeConstrainedEnabledCount == 1,
            "second recipient Time Constrained enable");
         publisher.enableTimeRegulation(publisherLookahead);
         publisherRegulating = true;
         await(publisher, callbackModel, () -> publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher Time Regulation enable");
         clock.enableTimeRegulation(clockLookahead);
         clockRegulating = true;
         await(clock, callbackModel, () -> clockRecorder.timeRegulationEnabledCount == 1,
            "clock Time Regulation enable");

         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(first.queryLogicalTime())
               && initial.equals(second.queryLogicalTime())
               && initial.equals(clock.queryLogicalTime()),
            "federates began at different logical times");
         LogicalTime messageTime = offset(initial, publisherFactory.makeEpsilon(), 6);
         LogicalTime firstTarget = offset(initial, firstFactory.makeEpsilon(), 6);
         LogicalTime secondTarget = offset(initial, secondFactory.makeEpsilon(), 10);
         LogicalTime clockTarget = offset(initial, clockFactory.makeEpsilon(), 2);
         byte[] payload = new byte[] {0x46, 0x41, 0x4e, 0x2d, 0x4f};
         byte[] tag = new byte[] {0x46, 0x41, 0x4e, 0x2d, 0x54, 0x53, 0x4f};
         ParameterHandleValueMap parameters = publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         hla.rti1516_2025.MessageRetractionReturn sent =
            publisher.sendInteraction(publisherInteraction, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped Send Interaction returned no valid retraction handle");
         MessageRetractionHandle retraction = sent.handle;
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRecorder.interactions.isEmpty()
               && secondRecorder.interactions.isEmpty(),
            "timestamped interaction arrived before either recipient frontier");

         first.timeAdvanceRequest(firstTarget);
         second.nextMessageRequest(secondTarget);
         JavaTckSupport.check(firstRecorder.timeAdvanceGrantTimes.isEmpty()
               && secondRecorder.timeAdvanceGrantTimes.isEmpty(),
            "recipient advanced before the producer frontier");
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisherRegulating = false;
         JavaTckSupport.expectFailure(() -> publisher.retract(retraction),
            "FederateNotExecutionMember", "retracting through the resigned producer");
         JavaTckSupport.check(firstRecorder.interactions.isEmpty()
               && secondRecorder.interactions.isEmpty()
               && firstRecorder.timeAdvanceGrantTimes.isEmpty()
               && secondRecorder.timeAdvanceGrantTimes.isEmpty(),
            "queued interactions were released before the recipients became safe to advance");

         clock.timeAdvanceRequest(clockTarget);
         await(clock, callbackModel, () -> clockRecorder.timeAdvanceGrantTimes.size() == 1,
            "clock frontier grant");
         await(first, callbackModel, () -> firstRecorder.interactions.size() == 1
               && firstRecorder.timeAdvanceGrantTimes.size() == 1,
            "first recipient delivery and grant");
         await(second, callbackModel, () -> secondRecorder.interactions.size() == 1
               && secondRecorder.timeAdvanceGrantTimes.size() == 1,
            "second recipient delivery and grant");

         checkInteraction(first, firstRecorder, firstInteraction, firstParameter, payload, tag,
            producer, messageTime, firstTarget, retraction);
         checkInteraction(second, secondRecorder, secondInteraction, secondParameter, payload, tag,
            producer, messageTime, messageTime, retraction);

         first.disableTimeConstrained();
         firstConstrained = false;
         second.disableTimeConstrained();
         secondConstrained = false;
         clock.disableTimeRegulation();
         clockRegulating = false;
      } finally {
         if (firstConstrained && firstJoined) {
            try { first.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (secondConstrained && secondJoined) {
            try { second.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (clockRegulating && clockJoined) {
            try { clock.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (firstSubscribed && firstJoined) {
            try { first.unsubscribeInteractionClass(
               first.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
         }
         if (secondSubscribed && secondJoined) {
            try { second.unsubscribeInteractionClass(
               second.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(
               publisher.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
         }
         if (firstJoined) {
            try { first.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (secondJoined) {
            try { second.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (clockJoined) {
            try { clock.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (clockConnected) { try { clock.disconnect(); } catch (Exception ignored) { } }
         if (secondConnected) { try { second.disconnect(); } catch (Exception ignored) { } }
         if (firstConnected) { try { first.disconnect(); } catch (Exception ignored) { } }
         if (publisherConnected) { try { publisher.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void checkInteraction(RTIambassador receiver, Recorder recorder,
         InteractionClassHandle expectedInteraction, ParameterHandle expectedParameter,
         byte[] payload, byte[] tag, FederateHandle producer, LogicalTime messageTime,
         LogicalTime expectedGrant, MessageRetractionHandle retraction) throws Exception {
      JavaTckSupport.check(recorder.interactions.size() == 1,
         "recipient received a duplicate or missing timestamped interaction");
      Interaction actual = recorder.interactions.get(0);
      JavaTckSupport.check(actual.interaction.equals(expectedInteraction)
            && actual.parameters.size() == 1
            && Arrays.equals(payload, actual.parameters.get(expectedParameter)),
         "recipient received the wrong interaction class or parameter");
      JavaTckSupport.check(Arrays.equals(tag, actual.tag) && producer.equals(actual.producer)
            && messageTime.equals(actual.time),
         "interaction callback did not preserve tag, producer, or timestamp");
      JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(actual.retraction),
         "interaction callback returned incorrect order or retraction metadata");
      JavaTckSupport.check(actual.transportation != null
            && !receiver.getTransportationTypeName(actual.transportation).isEmpty(),
         "interaction callback returned an unresolvable transportation handle");
      JavaTckSupport.check(actual.regions == null || actual.regions.isEmpty(),
         "ordinary timestamped interaction unexpectedly carried source regions");
      JavaTckSupport.check(recorder.timeAdvanceGrantTimes.size() == 1
            && expectedGrant.equals(recorder.timeAdvanceGrantTimes.get(0)),
         "recipient was granted an unexpected logical time");
      JavaTckSupport.check(recorder.callbackOrder.equals(
            Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
         "recipient received its interaction after the advance grant");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval start, int steps)
         throws Exception {
      LogicalTimeInterval result = start;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(start);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
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
      private int timeRegulationEnabledCount;
      private int timeConstrainedEnabledCount;
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabledCount;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Interaction interaction = new Interaction();
                  interaction.interaction = (InteractionClassHandle) arguments[0];
                  interaction.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
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
                  if ("toString".equals(name)) {
                     return "Java TCK interaction source-resignation fan-out callbacks";
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
