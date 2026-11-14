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
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Checks the Table 7 S-leaf Change Interaction Order Type request through the Java API. */
final class FederationMomChangeInteractionOrderTypeTck {
   private static final String TAKE_ORDER = "HLAinteractionRoot.ServerAction.TakeOrder";
   private static final String CHANGE_ORDER =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAchangeInteractionOrderType";
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;

   private FederationMomChangeInteractionOrderTypeTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "Change Interaction Order Type requires adapter-supplied logical-time configuration");
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-change-interaction-order-" + suffix;
      String subjectName = "java-tck-mom-change-interaction-subject-" + suffix;
      String observerName = "java-tck-mom-change-interaction-observer-" + suffix;
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      boolean interactionPublished = false;
      boolean interactionSubscribed = false;
      boolean requestPublished = false;
      InteractionClassHandle interaction = null;
      InteractionClassHandle request = null;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution(subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         interaction = subject.getInteractionClassHandle(TAKE_ORDER);
         InteractionClassHandle observerInteraction = observer.getInteractionClassHandle(TAKE_ORDER);
         JavaTckSupport.check(interaction != null && interaction.equals(observerInteraction),
            "the adapter FOM did not provide the standard Restaurant TakeOrder interaction");
         subject.publishInteractionClass(interaction);
         interactionPublished = true;
         observer.subscribeInteractionClass(observerInteraction);
         interactionSubscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = subject.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         subject.enableTimeRegulation(epsilon);
         await(subject, callbackModel, () -> subjectRecorder.timeRegulationEnabled,
            "sender time-regulation callback");
         LogicalTime timestamp = offset(subject.queryLogicalTime(), epsilon, 5);

         request = subject.getInteractionClassHandle(CHANGE_ORDER);
         ParameterHandle targetFederateParameter =
            subject.getParameterHandle(request, "HLAfederate");
         ParameterHandle interactionParameter =
            subject.getParameterHandle(request, "HLAinteractionClass");
         ParameterHandle orderParameter = subject.getParameterHandle(request, "HLAsendOrder");
         JavaTckSupport.check(request != null && targetFederateParameter != null
               && interactionParameter != null
               && orderParameter != null,
            "the standard MIM omitted Change Interaction Order Type request handles");
         subject.publishInteractionClass(request);
         requestPublished = true;

         FederateHandle targetFederate = subject.getFederateHandle(subjectName);
         JavaTckSupport.check(targetFederate != null,
            "the subject's federate handle could not be encoded for HLAchangeInteractionOrderType");
         byte[] encodedInteraction = encode(interaction);
         byte[] encodedTargetFederate = encode(targetFederate);
         byte[] receiveOrder = factory.getEncoderFactory().createHLAinteger32BE(1).toByteArray();
         ParameterHandleValueMap requestParameters =
            subject.getParameterHandleValueMapFactory().create(3);
         requestParameters.put(targetFederateParameter, encodedTargetFederate);
         requestParameters.put(interactionParameter, encodedInteraction);
         requestParameters.put(orderParameter, receiveOrder);
         subject.sendInteraction(request, requestParameters, new byte[0]);

         byte[] tag = new byte[] {0x49, 0x4E, 0x54, 0x4F};
         ParameterHandleValueMap noParameters =
            subject.getParameterHandleValueMapFactory().create(0);
         MessageRetractionReturn sendResult =
            subject.sendInteraction(interaction, noParameters, tag, timestamp);
         JavaTckSupport.check(sendResult != null && !sendResult.retractionHandleIsValid,
            "the MOM-selected RECEIVE order returned a retraction designator");

         await(observer, callbackModel, () -> !observerRecorder.timestampedInteractions.isEmpty(),
            "timestamped TakeOrder delivery after the MOM order-change request");
         settle(observer, callbackModel);
         JavaTckSupport.check(observerRecorder.timestampedInteractions.size() == 1
               && observerRecorder.receiveOrderInteractions.isEmpty(),
            "the timestamp-bearing TakeOrder did not use exactly one timestamped callback");
         InteractionDelivery delivery = observerRecorder.timestampedInteractions.get(0);
         JavaTckSupport.check(interaction.equals(delivery.interaction)
               && delivery.parameters.isEmpty() && Arrays.equals(tag, delivery.tag)
               && timestamp.equals(delivery.time)
               && subject.getFederateHandle(subjectName).equals(delivery.producer)
               && delivery.sentOrder == OrderType.RECEIVE
               && delivery.receivedOrder == OrderType.RECEIVE
               && delivery.retraction == null && delivery.regionsEmpty,
            "HLAchangeInteractionOrderType did not produce the expected RECEIVE-order delivery");

         subject.disableTimeRegulation();
         observer.unsubscribeInteractionClass(observerInteraction);
         interactionSubscribed = false;
         subject.unpublishInteractionClass(interaction);
         interactionPublished = false;
         subject.unpublishInteractionClass(request);
         requestPublished = false;
      } finally {
         if (requestPublished && request != null) {
            try { subject.unpublishInteractionClass(request); } catch (Exception ignored) { }
         }
         if (interactionSubscribed && interaction != null) {
            try { observer.unsubscribeInteractionClass(interaction); }
            catch (Exception ignored) { }
         }
         if (interactionPublished && interaction != null) {
            try { subject.unpublishInteractionClass(interaction); } catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static byte[] encode(InteractionClassHandle interaction) throws Exception {
      byte[] encoded = new byte[interaction.encodedLength()];
      interaction.encode(encoded, 0);
      return encoded;
   }

   private static byte[] encode(FederateHandle federate) throws Exception {
      byte[] encoded = new byte[federate.encodedLength()];
      federate.encode(encoded, 0);
      return encoded;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
   }

   private static void settle(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.sleep(25L);
   }

   private static final class InteractionDelivery {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private FederateHandle producer;
      private boolean regionsEmpty;
      private LogicalTime time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private final List<InteractionDelivery> timestampedInteractions = new ArrayList<>();
      private final List<InteractionDelivery> receiveOrderInteractions = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && (arguments.length == 6 || arguments.length == 10)) {
                  InteractionDelivery delivery = new InteractionDelivery();
                  delivery.interaction = (InteractionClassHandle) arguments[0];
                  delivery.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  delivery.tag = ((byte[]) arguments[2]).clone();
                  delivery.producer = (FederateHandle) arguments[4];
                  delivery.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  if (arguments.length == 10) {
                     delivery.time = (LogicalTime) arguments[6];
                     delivery.sentOrder = (OrderType) arguments[7];
                     delivery.receivedOrder = (OrderType) arguments[8];
                     delivery.retraction = (MessageRetractionHandle) arguments[9];
                     timestampedInteractions.add(delivery);
                  } else {
                     receiveOrderInteractions.add(delivery);
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK MOM order-change callbacks";
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
