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
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** Portable timestamped interaction delivery through the three alternate advances. */
final class TimestampedInteractionMixedAdvancesTck {
   private TimestampedInteractionMixedAdvancesTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador flushReceiver = factory.getRtiAmbassador();
      RTIambassador taraReceiver = factory.getRtiAmbassador();
      RTIambassador nmraReceiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder flushRecorder = new Recorder();
      Recorder taraRecorder = new Recorder();
      Recorder nmraRecorder = new Recorder();
      String federation = "java-tck-timestamped-interaction-mixed-advances";
      String uniqueFederation = federation + "-" + java.util.UUID.randomUUID();
      boolean publisherConnected = false;
      boolean flushConnected = false;
      boolean taraConnected = false;
      boolean nmraConnected = false;
      boolean publisherJoined = false;
      boolean flushJoined = false;
      boolean taraJoined = false;
      boolean nmraJoined = false;
      boolean created = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         flushReceiver.connect(flushRecorder.proxy(), callbackModel);
         flushConnected = true;
         taraReceiver.connect(taraRecorder.proxy(), callbackModel);
         taraConnected = true;
         nmraReceiver.connect(nmraRecorder.proxy(), callbackModel);
         nmraConnected = true;

         publisher.createFederationExecution(uniqueFederation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-interaction-producer", uniqueFederation);
         publisherJoined = true;
         flushReceiver.joinFederationExecution("java-tck-interaction-fq", uniqueFederation);
         flushJoined = true;
         taraReceiver.joinFederationExecution("java-tck-interaction-tara", uniqueFederation);
         taraJoined = true;
         nmraReceiver.joinFederationExecution("java-tck-interaction-nmra", uniqueFederation);
         nmraJoined = true;

         InteractionClassHandle publisherClass =
            publisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherClass, parameterName);
         publisher.publishInteractionClass(publisherClass);
         publisher.changeInteractionOrderType(publisherClass, OrderType.TIMESTAMP);

         InteractionClassHandle flushClass =
            flushReceiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle flushParameter =
            flushReceiver.getParameterHandle(flushClass, parameterName);
         InteractionClassHandle taraClass =
            taraReceiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle taraParameter = taraReceiver.getParameterHandle(taraClass, parameterName);
         InteractionClassHandle nmraClass =
            nmraReceiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle nmraParameter = nmraReceiver.getParameterHandle(nmraClass, parameterName);
         flushReceiver.subscribeInteractionClass(flushClass);
         taraReceiver.subscribeInteractionClass(taraClass);
         nmraReceiver.subscribeInteractionClass(nmraClass);

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         String timeName = publisherTimeFactory.getName();
         JavaTckSupport.check(timeName.equals(flushReceiver.getTimeFactory().getName())
               && timeName.equals(taraReceiver.getTimeFactory().getName())
               && timeName.equals(nmraReceiver.getTimeFactory().getName()),
            "timestamped interaction receivers selected different logical-time factories");

         publisher.enableTimeRegulation(epsilon);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabled,
            "timeRegulationEnabled did not cross the standard callback interface");
         for (RTIambassador receiver :
               Arrays.asList(flushReceiver, taraReceiver, nmraReceiver)) {
            receiver.enableTimeConstrained();
            JavaTckSupport.drain(receiver);
         }
         JavaTckSupport.check(flushRecorder.timeConstrainedEnabled
               && taraRecorder.timeConstrainedEnabled && nmraRecorder.timeConstrainedEnabled,
            "timeConstrainedEnabled did not cross every receiver callback interface");

         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime messageTime = offset(initial, epsilon, 5);
         LogicalTime publisherTarget = offset(initial, epsilon, 4);
         LogicalTime requestBoundary = offset(initial, epsilon, 10);
         byte[] payload = new byte[] {0x49, 0x4e, 0x54, 0x2d, 0x41};
         byte[] tag = new byte[] {0x49, 0x4e, 0x54, 0x2d, 0x46};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         MessageRetractionReturn retraction = publisher.sendInteraction(
            publisherClass, parameters, tag, messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Send Interaction returned no valid retraction handle");

         for (RTIambassador receiver :
               Arrays.asList(flushReceiver, taraReceiver, nmraReceiver)) {
            JavaTckSupport.drain(receiver);
         }
         JavaTckSupport.check(flushRecorder.timedInteractionCount == 0
               && taraRecorder.timedInteractionCount == 0
               && nmraRecorder.timedInteractionCount == 0,
            "timestamped interaction was delivered before an advance request");

         flushReceiver.flushQueueRequest(requestBoundary);
         taraReceiver.timeAdvanceRequestAvailable(messageTime);
         nmraReceiver.nextMessageRequestAvailable(requestBoundary);
         publisher.timeAdvanceRequest(publisherTarget);
         for (RTIambassador ambassador :
               Arrays.asList(publisher, flushReceiver, taraReceiver, nmraReceiver)) {
            JavaTckSupport.drain(ambassador);
         }

         checkReceiver(flushRecorder, flushClass, flushParameter, payload, tag,
            producer, messageTime, retraction.handle, OrderType.TIMESTAMP,
            "flush-queue receiver");
         checkReceiver(taraRecorder, taraClass, taraParameter, payload, tag,
            producer, messageTime, retraction.handle, OrderType.TIMESTAMP,
            "time-advance-available receiver");
         checkReceiver(nmraRecorder, nmraClass, nmraParameter, payload, tag,
            producer, messageTime, retraction.handle, OrderType.TIMESTAMP,
            "next-message-available receiver");

         JavaTckSupport.check(publisherRecorder.timeAdvanceGrantTimes.size() == 1
               && publisherTarget.equals(publisherRecorder.timeAdvanceGrantTimes.get(0)),
            "producer TAR did not grant the requested logical time");
         JavaTckSupport.check(flushRecorder.flushQueueGrantTimes.size() == 1
               && flushRecorder.timeAdvanceGrantTimes.isEmpty()
               && flushRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "flushQueueGrant")),
            "Flush Queue Request did not deliver the interaction before its sole grant");
         JavaTckSupport.check(taraRecorder.flushQueueGrantTimes.isEmpty()
               && taraRecorder.timeAdvanceGrantTimes.size() == 1
               && taraRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "TARA did not deliver the interaction before its sole grant");
         JavaTckSupport.check(nmraRecorder.flushQueueGrantTimes.isEmpty()
               && nmraRecorder.timeAdvanceGrantTimes.size() == 1
               && nmraRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "NMRA did not deliver the interaction before its sole grant");
         JavaTckSupport.check(messageTime.equals(taraRecorder.timeAdvanceGrantTimes.get(0))
               && messageTime.equals(nmraRecorder.timeAdvanceGrantTimes.get(0)),
            "TARA or NMRA reported the wrong grant time");
         JavaTckSupport.check(flushRecorder.flushQueueGrantTime != null
               && flushRecorder.flushQueueOptimisticTime != null
               && messageTime.equals(flushRecorder.flushQueueOptimisticTime)
               && compareTime(flushRecorder.flushQueueGrantTime, initial) >= 0
               && compareTime(flushRecorder.flushQueueGrantTime, requestBoundary) <= 0
               && compareTime(flushRecorder.flushQueueOptimisticTime,
                  flushRecorder.flushQueueGrantTime) >= 0
               && flushRecorder.flushQueueGrantTime.equals(flushReceiver.queryLogicalTime()),
            "Flush Queue Grant/query returned inconsistent actual or optimistic time");
         JavaTckSupport.check(messageTime.equals(taraReceiver.queryLogicalTime())
               && messageTime.equals(nmraReceiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "logical-time queries did not agree with their respective grants");
      } finally {
         if (flushJoined) {
            try { flushReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (taraJoined) {
            try { taraReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (nmraJoined) {
            try { nmraReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(uniqueFederation); }
            catch (Exception ignored) { }
         }
         if (flushConnected) {
            try { flushReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (taraConnected) {
            try { taraReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (nmraConnected) {
            try { nmraReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compareTime(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   private static void checkReceiver(Recorder recorder,
         InteractionClassHandle expectedClass, ParameterHandle expectedParameter,
         byte[] expectedPayload, byte[] expectedTag, FederateHandle expectedProducer,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         OrderType expectedOrder, String description) {
      JavaTckSupport.check(recorder.timedInteractionCount == 1
            && expectedClass.equals(recorder.interactionClass)
            && recorder.parameters != null && recorder.parameters.size() == 1
            && Arrays.equals(expectedPayload, recorder.parameters.get(expectedParameter)),
         description + " received the wrong interaction class or parameter payload");
      JavaTckSupport.check(Arrays.equals(expectedTag, recorder.tag),
         description + " did not preserve the user tag");
      JavaTckSupport.check(expectedProducer.equals(recorder.producer)
            && expectedTime.equals(recorder.interactionTime),
         description + " did not preserve producer or logical timestamp");
      JavaTckSupport.check(recorder.sentOrder == expectedOrder
            && recorder.receivedOrder == expectedOrder,
         description + " did not report timestamp order at both boundaries");
      JavaTckSupport.check(expectedRetraction.equals(recorder.retractionHandle),
         description + " did not preserve the message retraction handle");
      JavaTckSupport.check(recorder.transportation != null,
         description + " returned no transportation handle");
   }

   private static final class Recorder {
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private int timedInteractionCount;
      private InteractionClassHandle interactionClass;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> interactionTime;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retractionHandle;
      private LogicalTime<?, ?> flushQueueGrantTime;
      private LogicalTime<?, ?> flushQueueOptimisticTime;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();

      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  timedInteractionCount++;
                  interactionClass = (InteractionClassHandle) arguments[0];
                  parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  tag = ((byte[]) arguments[2]).clone();
                  transportation = (TransportationTypeHandle) arguments[3];
                  producer = (FederateHandle) arguments[4];
                  interactionTime = (LogicalTime<?, ?>) arguments[6];
                  sentOrder = (OrderType) arguments[7];
                  receivedOrder = (OrderType) arguments[8];
                  retractionHandle = (MessageRetractionHandle) arguments[9];
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               } else if ("flushQueueGrant".equals(name)
                     && arguments != null && arguments.length == 2) {
                  flushQueueGrantTime = (LogicalTime<?, ?>) arguments[0];
                  flushQueueOptimisticTime = (LogicalTime<?, ?>) arguments[1];
                  flushQueueGrantTimes.add(flushQueueGrantTime);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped interaction callbacks";
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
