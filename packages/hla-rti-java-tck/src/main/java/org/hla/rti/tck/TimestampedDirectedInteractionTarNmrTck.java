package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
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
import java.util.UUID;

/** Delivers a timestamped directed interaction through independent TAR and NMR grants. */
final class TimestampedDirectedInteractionTarNmrTck {
   private TimestampedDirectedInteractionTarNmrTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador tarReceiver = factory.getRtiAmbassador();
      RTIambassador nmrReceiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder tarRecorder = new Recorder();
      Recorder nmrRecorder = new Recorder();
      String federation = "java-tck-timestamped-directed-tar-nmr-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean tarConnected = false;
      boolean nmrConnected = false;
      boolean publisherJoined = false;
      boolean tarJoined = false;
      boolean nmrJoined = false;
      boolean created = false;
      boolean attributesPublished = false;
      boolean directedPublished = false;
      boolean tarAttributesSubscribed = false;
      boolean nmrAttributesSubscribed = false;
      boolean tarDirectedSubscribed = false;
      boolean nmrDirectedSubscribed = false;
      boolean publisherRegulating = false;
      boolean tarConstrained = false;
      boolean nmrConstrained = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         tarReceiver.connect(tarRecorder.proxy(), callbackModel);
         tarConnected = true;
         nmrReceiver.connect(nmrRecorder.proxy(), callbackModel);
         nmrConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-tar-nmr-producer", federation);
         publisherJoined = true;
         tarReceiver.joinFederationExecution("java-tck-directed-tar-member", federation);
         tarJoined = true;
         nmrReceiver.joinFederationExecution("java-tck-directed-nmr-member", federation);
         nmrJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle tarClass = tarReceiver.getObjectClassHandle(objectClassName);
         ObjectClassHandle nmrClass = nmrReceiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle tarAttribute = tarReceiver.getAttributeHandle(tarClass, attributeName);
         AttributeHandle nmrAttribute = nmrReceiver.getAttributeHandle(nmrClass, attributeName);
         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet tarAttributes =
            JavaTckSupport.attributeSet(tarReceiver, tarAttribute);
         AttributeHandleSet nmrAttributes =
            JavaTckSupport.attributeSet(nmrReceiver, nmrAttribute);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle tarInteraction =
            tarReceiver.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle nmrInteraction =
            nmrReceiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle tarParameter = tarReceiver.getParameterHandle(tarInteraction, parameterName);
         ParameterHandle nmrParameter = nmrReceiver.getParameterHandle(nmrInteraction, parameterName);

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         attributesPublished = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         directedPublished = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         tarReceiver.subscribeObjectClassAttributes(tarClass, tarAttributes);
         tarAttributesSubscribed = true;
         nmrReceiver.subscribeObjectClassAttributes(nmrClass, nmrAttributes);
         nmrAttributesSubscribed = true;
         tarReceiver.subscribeObjectClassDirectedInteractions(tarClass,
            JavaTckSupport.interactionSet(tarReceiver, tarInteraction));
         tarDirectedSubscribed = true;
         nmrReceiver.subscribeObjectClassDirectedInteractions(nmrClass,
            JavaTckSupport.interactionSet(nmrReceiver, nmrInteraction));
         nmrDirectedSubscribed = true;

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.drain(tarReceiver);
         JavaTckSupport.drain(nmrReceiver);
         JavaTckSupport.check(tarRecorder.discoveries.contains(target)
               && nmrRecorder.discoveries.contains(target),
            "TAR and NMR receivers did not both discover the directed target");

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> tarTimeFactory = tarReceiver.getTimeFactory();
         LogicalTimeFactory<?, ?> nmrTimeFactory = nmrReceiver.getTimeFactory();
         String timeFactoryName = publisherTimeFactory.getName();
         JavaTckSupport.check(timeFactoryName.equals(tarTimeFactory.getName())
               && timeFactoryName.equals(nmrTimeFactory.getName()),
            "publisher and receivers selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTimeInterval publisherLookahead = offset(epsilon, 5);
         tarReceiver.enableTimeConstrained();
         tarConstrained = true;
         JavaTckSupport.drain(tarReceiver);
         nmrReceiver.enableTimeConstrained();
         nmrConstrained = true;
         JavaTckSupport.drain(nmrReceiver);
         JavaTckSupport.check(tarRecorder.timeConstrainedEnabledCount == 1
               && nmrRecorder.timeConstrainedEnabledCount == 1,
            "both directed receivers did not report Time Constrained enable");
         publisher.enableTimeRegulation(publisherLookahead);
         publisherRegulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher did not report Time Regulation enable");

         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(tarReceiver.queryLogicalTime())
               && initial.equals(nmrReceiver.queryLogicalTime()),
            "publisher and directed receivers began at different logical times");
         LogicalTime messageTime = offset(initial, epsilon, 7);
         LogicalTime tarTarget = offset(initial, epsilon, 7);
         LogicalTime nmrTarget = offset(initial, epsilon, 10);
         LogicalTime publisherTarget = offset(initial, epsilon, 2);
         byte[] payload = new byte[] {0x54, 0x41, 0x52, 0x2D, 0x4E};
         byte[] tag = new byte[] {0x54, 0x41, 0x52, 0x2D, 0x4D};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         MessageRetractionReturn sent = publisher.sendDirectedInteraction(
            publisherInteraction, target, parameters, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped directed send returned no valid retraction handle");
         JavaTckSupport.drain(tarReceiver);
         JavaTckSupport.drain(nmrReceiver);
         JavaTckSupport.check(tarRecorder.interactions.isEmpty()
               && nmrRecorder.interactions.isEmpty(),
            "timestamped directed interaction arrived before either advance request");

         tarReceiver.timeAdvanceRequest(tarTarget);
         nmrReceiver.nextMessageRequest(nmrTarget);
         JavaTckSupport.drain(tarReceiver);
         JavaTckSupport.drain(nmrReceiver);
         JavaTckSupport.check(tarRecorder.timeAdvanceGrants.isEmpty()
               && nmrRecorder.timeAdvanceGrants.isEmpty(),
            "a constrained receiver was granted before the publisher frontier");
         publisher.timeAdvanceRequest(publisherTarget);
         JavaTckSupport.drain(tarReceiver);
         JavaTckSupport.drain(nmrReceiver);
         JavaTckSupport.drain(publisher);

         JavaTckSupport.check(tarRecorder.interactions.size() == 1
               && nmrRecorder.interactions.size() == 1,
            "TAR and NMR did not each receive exactly one directed interaction");
         JavaTckSupport.check(publisherRecorder.timeAdvanceGrants.size() == 1
               && publisherTarget.equals(publisherRecorder.timeAdvanceGrants.get(0)),
            "publisher TAR did not grant the requested logical time");
         JavaTckSupport.check(tarRecorder.timeAdvanceGrants.size() == 1
               && tarTarget.equals(tarRecorder.timeAdvanceGrants.get(0)),
            "TAR receiver did not grant its requested logical time");
         JavaTckSupport.check(nmrRecorder.timeAdvanceGrants.size() == 1
               && messageTime.equals(nmrRecorder.timeAdvanceGrants.get(0)),
            "NMR receiver did not grant at the timestamped message time");
         JavaTckSupport.check(tarRecorder.callbackOrder.equals(
               Arrays.asList("receiveDirectedInteraction", "timeAdvanceGrant"))
               && nmrRecorder.callbackOrder.equals(
                  Arrays.asList("receiveDirectedInteraction", "timeAdvanceGrant")),
            "directed interaction delivery did not precede both receiver grants");
         checkInteraction(tarRecorder.interactions.get(0), tarInteraction, target, tarParameter,
            payload, tag, producer, messageTime, sent.handle, tarReceiver, "TAR receiver");
         checkInteraction(nmrRecorder.interactions.get(0), nmrInteraction, target, nmrParameter,
            payload, tag, producer, messageTime, sent.handle, nmrReceiver, "NMR receiver");
         JavaTckSupport.check(tarTarget.equals(tarReceiver.queryLogicalTime())
               && messageTime.equals(nmrReceiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "logical-time queries did not match the TAR, NMR, and producer grants");
         expectException(() -> publisher.retract(sent.handle), "MessageCanNoLongerBeRetracted",
            "retracting the directed interaction after both deliveries");

         tarReceiver.disableTimeConstrained();
         tarConstrained = false;
         nmrReceiver.disableTimeConstrained();
         nmrConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
      } finally {
         if (tarConstrained) {
            try { tarReceiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (nmrConstrained) {
            try { nmrReceiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (tarDirectedSubscribed && tarJoined) {
            try { tarReceiver.unsubscribeObjectClassDirectedInteractions(
               tarReceiver.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         }
         if (nmrDirectedSubscribed && nmrJoined) {
            try { nmrReceiver.unsubscribeObjectClassDirectedInteractions(
               nmrReceiver.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         }
         if (tarAttributesSubscribed && tarJoined) {
            try {
               ObjectClassHandle objectClass = tarReceiver.getObjectClassHandle(objectClassName);
               tarReceiver.unsubscribeObjectClassAttributes(objectClass,
                  JavaTckSupport.attributeSet(tarReceiver,
                     tarReceiver.getAttributeHandle(objectClass, attributeName)));
            } catch (Exception ignored) { }
         }
         if (nmrAttributesSubscribed && nmrJoined) {
            try {
               ObjectClassHandle objectClass = nmrReceiver.getObjectClassHandle(objectClassName);
               nmrReceiver.unsubscribeObjectClassAttributes(objectClass,
                  JavaTckSupport.attributeSet(nmrReceiver,
                     nmrReceiver.getAttributeHandle(objectClass, attributeName)));
            } catch (Exception ignored) { }
         }
         if (directedPublished && publisherJoined) {
            try { publisher.unpublishObjectClassDirectedInteractions(
               publisher.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         }
         if (attributesPublished && publisherJoined) {
            try {
               ObjectClassHandle objectClass = publisher.getObjectClassHandle(objectClassName);
               publisher.unpublishObjectClassAttributes(objectClass,
                  JavaTckSupport.attributeSet(publisher,
                     publisher.getAttributeHandle(objectClass, attributeName)));
            } catch (Exception ignored) { }
         }
         if (nmrJoined) {
            try { nmrReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (tarJoined) {
            try { tarReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (nmrConnected) {
            try { nmrReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (tarConnected) {
            try { tarReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTimeInterval offset(LogicalTimeInterval start, int steps)
         throws Exception {
      LogicalTimeInterval result = start;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(start);
      return result;
   }

   private static void checkInteraction(Interaction actual,
         InteractionClassHandle expectedClass, ObjectInstanceHandle expectedObject,
         ParameterHandle expectedParameter, byte[] expectedPayload, byte[] expectedTag,
         FederateHandle expectedProducer, LogicalTime<?, ?> expectedTime,
         MessageRetractionHandle expectedRetraction, RTIambassador inspector, String role)
         throws Exception {
      JavaTckSupport.check(expectedClass.equals(actual.interactionClass)
            && expectedObject.equals(actual.object) && actual.parameters != null
            && actual.parameters.size() == 1
            && Arrays.equals(expectedPayload, actual.parameters.get(expectedParameter)),
         role + " callback returned the wrong target or parameter");
      JavaTckSupport.check(Arrays.equals(expectedTag, actual.tag)
            && expectedProducer.equals(actual.producer) && expectedTime.equals(actual.time),
         role + " callback did not preserve tag, producer, or timestamp");
      JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP,
         role + " callback returned the wrong sent or received order");
      JavaTckSupport.check(expectedRetraction.equals(actual.retraction)
            && actual.transportation != null
            && !inspector.getTransportationTypeName(actual.transportation).isEmpty(),
         role + " callback omitted the retraction or resolvable transport handle");
   }

   private static void expectException(CheckedOperation operation, String expected,
         String description) throws Exception {
      try {
         operation.run();
      } catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass()) {
            if (expected.equals(type.getSimpleName())) return;
         }
         throw new AssertionError(description + " threw " + exception.getClass().getName()
            + " instead of " + expected, exception);
      }
      throw new AssertionError(description + " did not throw " + expected);
   }

   @FunctionalInterface
   private interface CheckedOperation { void run() throws Exception; }

   private static final class Interaction {
      private InteractionClassHandle interactionClass;
      private ObjectInstanceHandle object;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private int timeRegulationEnabledCount;
      private int timeConstrainedEnabledCount;
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrants = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabledCount;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
               } else if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  Interaction interaction = new Interaction();
                  interaction.interactionClass = (InteractionClassHandle) arguments[0];
                  interaction.object = (ObjectInstanceHandle) arguments[1];
                  interaction.parameters = ((ParameterHandleValueMap) arguments[2]).clone();
                  interaction.tag = ((byte[]) arguments[3]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[4];
                  interaction.producer = (FederateHandle) arguments[5];
                  interaction.time = (LogicalTime<?, ?>) arguments[6];
                  interaction.sentOrder = (OrderType) arguments[7];
                  interaction.receivedOrder = (OrderType) arguments[8];
                  interaction.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(interaction);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed TAR/NMR callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
