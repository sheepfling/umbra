package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.FederationExecutionMemberInformation;
import hla.rti1516_2025.FederationExecutionMemberInformationSet;
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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks multi-execution participation and receive-order and timestamped data isolation. */
final class FederateApplicationMultipleExecutionsTck {
   private static final String OBJECT_CLASS_NAME = "HLAobjectRoot.TckObject";
   private static final String ATTRIBUTE_NAME = "Value";
   private static final String INTERACTION_CLASS_NAME = "HLAinteractionRoot.TckInteraction";
   private static final String PARAMETER_NAME = "Payload";
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FederateApplicationMultipleExecutionsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String federateType, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador firstOne = factory.getRtiAmbassador();
      RTIambassador firstTwo = factory.getRtiAmbassador();
      RTIambassador secondOne = factory.getRtiAmbassador();
      RTIambassador secondTwo = factory.getRtiAmbassador();
      RTIambassador[] ambassadors = {creator, firstOne, firstTwo, secondOne, secondTwo};
      Recorder[] recorders = {new Recorder(), new Recorder(), new Recorder(),
         new Recorder(), new Recorder()};
      String suffix = UUID.randomUUID().toString();
      String firstFederation = "java-tck-multiple-executions-" + suffix;
      String secondFederation = caseVariant(firstFederation);
      String firstOneName = "multi-execution-first-one";
      String firstTwoName = caseVariant(firstOneName);
      String secondOneName = firstOneName;
      String secondTwoName = firstTwoName;

      try {
         JavaTckSupport.check(firstOne != firstTwo && firstOne != secondOne
               && firstOne != secondTwo && firstTwo != secondOne
               && firstTwo != secondTwo && secondOne != secondTwo,
            "one application received the same RTIambassador object for multiple federates");
         for (int i = 0; i < ambassadors.length; ++i) {
            ambassadors[i].connect(recorders[i].proxy(), callbackModel);
         }
         creator.createFederationExecution(firstFederation, fom, timeImplementation);
         JavaTckSupport.expectFailure(
            () -> creator.createFederationExecution(firstFederation, fom, timeImplementation),
            "FederationExecutionAlreadyExists",
            "creating an execution with an exact duplicate name");
         creator.createFederationExecution(secondFederation, fom, timeImplementation);
         creator.listFederationExecutions();
         awaitSingle(creator, recorders[0], callbackModel,
            () -> recorders[0].hasFederations(
               firstFederation, secondFederation, timeImplementation),
            "case-variant federation execution report");

         FederateHandle firstOneHandle = firstOne.joinFederationExecution(
            firstOneName, federateType, firstFederation);
         FederateHandle secondOneHandle = secondOne.joinFederationExecution(
            secondOneName, federateType, secondFederation);
         JavaTckSupport.expectFailure(
            () -> firstTwo.joinFederationExecution(firstOneName, federateType, firstFederation),
            "FederateNameAlreadyInUse",
            "joining with an exact duplicate federate name");
         FederateHandle firstTwoHandle = firstTwo.joinFederationExecution(
            firstTwoName, federateType, firstFederation);
         FederateHandle secondTwoHandle = secondTwo.joinFederationExecution(
            secondTwoName, federateType, secondFederation);
         checkHandle(firstOneHandle, "first execution first federate");
         checkHandle(secondOneHandle, "second execution first federate");
         checkHandle(firstTwoHandle, "first execution second federate");
         checkHandle(secondTwoHandle, "second execution second federate");
         firstOne.listFederationExecutionMembers(firstFederation);
         awaitSingle(firstOne, recorders[1], callbackModel,
            () -> recorders[1].hasMembers(
               firstFederation, firstOneName, firstTwoName, federateType),
            "case-variant federate member report");
         creator.listFederationExecutionMembers(secondFederation);
         awaitSingle(creator, recorders[0], callbackModel,
            () -> recorders[0].hasMembers(
               secondFederation, secondOneName, secondTwoName, federateType),
            "the second execution's independent member set with the same federate names");

         verifyExecutionDataIsolation(
            firstOne, firstTwo, secondOne, secondTwo,
            recorders[1], recorders[2], recorders[3], recorders[4],
            firstOneHandle, fom, timeImplementation, callbackModel);

         firstTwo.resignFederationExecution(ResignAction.NO_ACTION);
         firstTwoHandle = firstTwo.joinFederationExecution(
            "multi-execution-first-two-rejoined", federateType, secondFederation);
         checkHandle(firstTwoHandle, "reused RTIambassador in second execution");

         secondOne.resignFederationExecution(ResignAction.NO_ACTION);
         firstOne.resignFederationExecution(ResignAction.NO_ACTION);
         secondTwo.resignFederationExecution(ResignAction.NO_ACTION);
         firstTwo.resignFederationExecution(ResignAction.NO_ACTION);
         creator.destroyFederationExecution(firstFederation);
         creator.destroyFederationExecution(secondFederation);
      } finally {
         for (RTIambassador ambassador : ambassadors) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         try { creator.destroyFederationExecution(firstFederation); }
         catch (Exception ignored) { }
         try { creator.destroyFederationExecution(secondFederation); }
         catch (Exception ignored) { }
         for (RTIambassador ambassador : ambassadors) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void verifyExecutionDataIsolation(
         RTIambassador publisher, RTIambassador localSubscriber,
         RTIambassador otherSubscriber, RTIambassador otherRegulator,
         Recorder publisherRecorder, Recorder localRecorder,
         Recorder otherSubscriberRecorder, Recorder otherRegulatorRecorder,
         FederateHandle producer, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador[] participants = {
         publisher, localSubscriber, otherSubscriber, otherRegulator
      };
      Recorder[] participantRecorders = {
         publisherRecorder, localRecorder, otherSubscriberRecorder, otherRegulatorRecorder
      };
      ObjectClassHandle publisherClass = publisher.getObjectClassHandle(OBJECT_CLASS_NAME);
      ObjectClassHandle localClass = localSubscriber.getObjectClassHandle(OBJECT_CLASS_NAME);
      ObjectClassHandle otherClass = otherSubscriber.getObjectClassHandle(OBJECT_CLASS_NAME);
      AttributeHandle publisherAttribute =
         publisher.getAttributeHandle(publisherClass, ATTRIBUTE_NAME);
      AttributeHandle localAttribute = localSubscriber.getAttributeHandle(localClass, ATTRIBUTE_NAME);
      AttributeHandle otherAttribute = otherSubscriber.getAttributeHandle(otherClass, ATTRIBUTE_NAME);
      InteractionClassHandle publisherInteraction =
         publisher.getInteractionClassHandle(INTERACTION_CLASS_NAME);
      InteractionClassHandle localInteraction =
         localSubscriber.getInteractionClassHandle(INTERACTION_CLASS_NAME);
      InteractionClassHandle otherInteraction =
         otherSubscriber.getInteractionClassHandle(INTERACTION_CLASS_NAME);
      ParameterHandle publisherParameter =
         publisher.getParameterHandle(publisherInteraction, PARAMETER_NAME);
      ParameterHandle localParameter =
         localSubscriber.getParameterHandle(localInteraction, PARAMETER_NAME);
      ParameterHandle otherParameter =
         otherSubscriber.getParameterHandle(otherInteraction, PARAMETER_NAME);
      JavaTckSupport.check(publisherClass != null && localClass != null && otherClass != null
            && publisherAttribute != null && localAttribute != null && otherAttribute != null
            && publisherInteraction != null && localInteraction != null
            && otherInteraction != null && publisherParameter != null
            && localParameter != null && otherParameter != null,
         "independent executions did not expose the selected FOM declarations");

      AttributeHandleSet publishedAttributes =
         publisher.getAttributeHandleSetFactory().create();
      publishedAttributes.add(publisherAttribute);
      AttributeHandleSet localAttributes =
         localSubscriber.getAttributeHandleSetFactory().create();
      localAttributes.add(localAttribute);
      AttributeHandleSet otherAttributes =
         otherSubscriber.getAttributeHandleSetFactory().create();
      otherAttributes.add(otherAttribute);
      boolean publishedObject = false;
      boolean localSubscribed = false;
      boolean otherSubscribed = false;
      boolean publishedInteraction = false;
      boolean localInteractionSubscribed = false;
      boolean otherInteractionSubscribed = false;
      boolean localConstrained = false;
      boolean otherConstrained = false;
      boolean publisherRegulating = false;
      boolean otherRegulating = false;
      try {
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes);
         publishedObject = true;
         localSubscriber.subscribeObjectClassAttributes(localClass, localAttributes);
         localSubscribed = true;
         otherSubscriber.subscribeObjectClassAttributes(otherClass, otherAttributes);
         otherSubscribed = true;
         publisher.publishInteractionClass(publisherInteraction);
         publishedInteraction = true;
         localSubscriber.subscribeInteractionClass(localInteraction);
         localInteractionSubscribed = true;
         otherSubscriber.subscribeInteractionClass(otherInteraction);
         otherInteractionSubscribed = true;

         int otherDiscoveryCount = otherSubscriberRecorder.discoveries.size();
         int otherReflectionCount = otherSubscriberRecorder.reflections.size();
         int otherReceiveOrderCount = otherSubscriberRecorder.receiveOrderInteractions.size();
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null,
            "same-execution isolation control registered no object instance");
         await(participants, participantRecorders, callbackModel,
            () -> localRecorder.discoveries.contains(object),
            "same-execution object discovery");

         byte[] attributeValue = new byte[] {0x2a};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, attributeValue.clone());
         int localReflectionCount = localRecorder.reflections.size();
         publisher.updateAttributeValues(object, values, new byte[0]);
         await(participants, participantRecorders, callbackModel,
            () -> localRecorder.reflections.size() > localReflectionCount,
            "same-execution attribute reflection");
         Reflection reflection = localRecorder.reflections.get(localRecorder.reflections.size() - 1);
         JavaTckSupport.check(object.equals(reflection.object)
               && reflection.values.containsKey(localAttribute)
               && Arrays.equals(attributeValue, reflection.values.get(localAttribute)),
            "same-execution control did not reflect the published attribute value");

         byte[] receiveOrderPayload = new byte[] {0x2b};
         ParameterHandleValueMap receiveOrderParameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         receiveOrderParameters.put(publisherParameter, receiveOrderPayload.clone());
         int localInteractionCount = localRecorder.receiveOrderInteractions.size();
         publisher.sendInteraction(
            publisherInteraction, receiveOrderParameters, new byte[0]);
         await(participants, participantRecorders, callbackModel,
            () -> localRecorder.receiveOrderInteractions.size() > localInteractionCount,
            "same-execution receive-order interaction");
         Interaction received = localRecorder.receiveOrderInteractions.get(
            localRecorder.receiveOrderInteractions.size() - 1);
         JavaTckSupport.check(localInteraction.equals(received.interaction)
               && received.parameters.size() == 1
               && Arrays.equals(receiveOrderPayload, received.parameters.get(localParameter))
               && producer.equals(received.producer),
            "same-execution control did not deliver the published receive-order interaction");

         awaitIsolationWindow(otherSubscriber, otherSubscriberRecorder, callbackModel,
            otherDiscoveryCount, otherReflectionCount, otherReceiveOrderCount,
            "receive-order object or interaction data crossed into another execution");

         JavaTckSupport.check(fom != null && !fom.isEmpty(),
            "multiple-execution data isolation requires an adapter-supplied FOM");
         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> localFactory = localSubscriber.getTimeFactory();
         LogicalTimeFactory<?, ?> otherRegulatorFactory = otherRegulator.getTimeFactory();
         LogicalTimeFactory<?, ?> otherSubscriberFactory = otherSubscriber.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(timeImplementation)
               && publisherFactory.getName().equals(localFactory.getName())
               && publisherFactory.getName().equals(otherRegulatorFactory.getName())
               && publisherFactory.getName().equals(otherSubscriberFactory.getName()),
            "cross-execution TSO participants selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = publisherFactory.makeEpsilon();
         localSubscriber.enableTimeConstrained();
         localConstrained = true;
         otherSubscriber.enableTimeConstrained();
         otherConstrained = true;
         publisher.enableTimeRegulation(epsilon);
         publisherRegulating = true;
         otherRegulator.enableTimeRegulation(epsilon);
         otherRegulating = true;
         await(participants, participantRecorders, callbackModel,
            () -> localRecorder.timeConstrainedEnabled
               && otherSubscriberRecorder.timeConstrainedEnabled
               && publisherRecorder.timeRegulationEnabled
               && otherRegulatorRecorder.timeRegulationEnabled,
            "time-constrained and time-regulating callbacks");

         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         clearTimeEvents(participantRecorders);
         LogicalTime<?, ?> messageTime = offset(publisherInitial, epsilon, 3);
         LogicalTime<?, ?> grantTime = offset(publisherInitial, epsilon, 4);
         byte[] timestampedPayload = new byte[] {0x2c, 0x2d};
         byte[] timestampedTag = new byte[] {0x54, 0x53, 0x4f};
         ParameterHandleValueMap timestampedParameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         timestampedParameters.put(publisherParameter, timestampedPayload.clone());
         MessageRetractionReturn retraction = publisher.sendInteraction(
            publisherInteraction, timestampedParameters, timestampedTag.clone(), messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "cross-execution timestamped interaction send returned no valid retraction handle");

         long pregrantDeadline = System.nanoTime() + 32_000_000L;
         while (System.nanoTime() < pregrantDeadline) {
            service(otherSubscriber, callbackModel);
            Thread.sleep(1);
         }
         publisher.timeAdvanceRequest(grantTime);
         localSubscriber.timeAdvanceRequest(grantTime);
         otherRegulator.timeAdvanceRequest(grantTime);
         otherSubscriber.timeAdvanceRequest(grantTime);
         await(participants, participantRecorders, callbackModel,
            () -> publisherRecorder.grants.size() == 1
               && localRecorder.grants.size() == 1
               && localRecorder.timestampedInteractions.size() == 1
               && otherRegulatorRecorder.grants.size() == 1
               && otherSubscriberRecorder.grants.size() == 1,
            "same-execution TSO delivery and cross-execution grant");

         TimedInteraction timestamped = localRecorder.timestampedInteractions.get(0);
         JavaTckSupport.check(localInteraction.equals(timestamped.interaction)
               && timestamped.parameters.size() == 1
               && Arrays.equals(timestampedPayload, timestamped.parameters.get(localParameter))
               && Arrays.equals(timestampedTag, timestamped.tag)
               && producer.equals(timestamped.producer)
               && messageTime.equals(timestamped.time)
               && timestamped.sentOrder == OrderType.TIMESTAMP
               && timestamped.receivedOrder == OrderType.TIMESTAMP
               && timestamped.transportation != null
               && !localSubscriber.getTransportationTypeName(timestamped.transportation).isEmpty()
               && retraction.handle.equals(timestamped.retraction),
            "same-execution TSO control did not preserve interaction data and timestamp metadata");
         JavaTckSupport.check(grantTime.equals(publisherRecorder.grants.get(0))
               && grantTime.equals(localRecorder.grants.get(0))
               && grantTime.equals(otherRegulatorRecorder.grants.get(0))
               && grantTime.equals(otherSubscriberRecorder.grants.get(0)),
            "cross-execution TSO participants received the wrong grant time");
         JavaTckSupport.check(localRecorder.callbackOrder.equals(
               Arrays.asList("receiveInteraction", "timeAdvanceGrant")),
            "same-execution TSO interaction was not delivered before its grant");
         awaitIsolationWindow(otherSubscriber, otherSubscriberRecorder, callbackModel,
            otherDiscoveryCount, otherReflectionCount, otherReceiveOrderCount,
            "timestamped interaction crossed into another execution");
         JavaTckSupport.check(otherSubscriberRecorder.timestampedInteractions.isEmpty()
               && otherSubscriberRecorder.callbackOrder.equals(
                  Arrays.asList("timeAdvanceGrant")),
            "other execution received a timestamped interaction instead of only its grant");
      } finally {
         if (localConstrained) {
            try { localSubscriber.disableTimeConstrained(); }
            catch (Exception ignored) { }
         }
         if (otherConstrained) {
            try { otherSubscriber.disableTimeConstrained(); }
            catch (Exception ignored) { }
         }
         if (publisherRegulating) {
            try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         }
         if (otherRegulating) {
            try { otherRegulator.disableTimeRegulation(); }
            catch (Exception ignored) { }
         }
         if (otherInteractionSubscribed) {
            try { otherSubscriber.unsubscribeInteractionClass(otherInteraction); }
            catch (Exception ignored) { }
         }
         if (localInteractionSubscribed) {
            try { localSubscriber.unsubscribeInteractionClass(localInteraction); }
            catch (Exception ignored) { }
         }
         if (publishedInteraction) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (otherSubscribed) {
            try { otherSubscriber.unsubscribeObjectClassAttributes(otherClass, otherAttributes); }
            catch (Exception ignored) { }
         }
         if (localSubscribed) {
            try { localSubscriber.unsubscribeObjectClassAttributes(localClass, localAttributes); }
            catch (Exception ignored) { }
         }
         if (publishedObject) {
            try { publisher.unpublishObjectClass(publisherClass); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void awaitIsolationWindow(RTIambassador otherSubscriber, Recorder recorder,
         CallbackModel callbackModel, int discoveryCount, int reflectionCount,
         int receiveOrderCount, String description) throws Exception {
      long deadline = System.nanoTime() + 500_000_000L;
      while (System.nanoTime() < deadline
            && recorder.discoveries.size() == discoveryCount
            && recorder.reflections.size() == reflectionCount
            && recorder.receiveOrderInteractions.size() == receiveOrderCount
            && recorder.timestampedInteractions.isEmpty()) {
         service(otherSubscriber, callbackModel);
         Thread.sleep(1);
      }
      JavaTckSupport.check(recorder.discoveries.size() == discoveryCount
            && recorder.reflections.size() == reflectionCount
            && recorder.receiveOrderInteractions.size() == receiveOrderCount
            && recorder.timestampedInteractions.isEmpty(),
         description);
   }

   private static void await(RTIambassador[] ambassadors, Recorder[] recorders,
         CallbackModel callbackModel, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         for (RTIambassador ambassador : ambassadors) service(ambassador, callbackModel);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 0.001);
      } else {
         ambassador.getObjectClassHandle(OBJECT_CLASS_NAME);
      }
   }

   private static void awaitSingle(RTIambassador ambassador, Recorder recorder,
         CallbackModel callbackModel, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         service(ambassador, callbackModel);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static String caseVariant(String name) {
      for (int i = 0; i < name.length(); ++i) {
         char character = name.charAt(i);
         if (character >= 'a' && character <= 'z') {
            return name.substring(0, i)
               + (char) ('A' + (character - 'a')) + name.substring(i + 1);
         }
      }
      throw new IllegalArgumentException("name has no ASCII lowercase letter to vary");
   }

   private static void clearTimeEvents(Recorder[] recorders) {
      for (Recorder recorder : recorders) {
         recorder.grants.clear();
         recorder.timestampedInteractions.clear();
         recorder.callbackOrder.clear();
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval<?> interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = result.add(interval);
      return result;
   }

   private static void checkHandle(FederateHandle handle, String description) {
      JavaTckSupport.check(handle != null && handle.encodedLength() > 0,
         description + " returned an invalid FederateHandle");
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new HashMap<>();

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap receivedValues = (AttributeHandleValueMap) arguments[1];
         receivedValues.forEach((handle, value) -> values.put(handle, value.clone()));
      }
   }

   private static class Interaction {
      final InteractionClassHandle interaction;
      final Map<ParameterHandle, byte[]> parameters = new HashMap<>();
      final byte[] tag;
      final TransportationTypeHandle transportation;
      final FederateHandle producer;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap receivedParameters = (ParameterHandleValueMap) arguments[1];
         receivedParameters.forEach((handle, value) -> parameters.put(handle, value.clone()));
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
      }
   }

   private static final class TimedInteraction extends Interaction {
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private TimedInteraction(Object[] arguments) {
         super(arguments);
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Interaction> receiveOrderInteractions = new CopyOnWriteArrayList<>();
      private final List<TimedInteraction> timestampedInteractions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final List<FederationExecutionInformationSet> federationReports =
         new CopyOnWriteArrayList<>();
      private final List<MemberReport> memberReports = new CopyOnWriteArrayList<>();

      private boolean hasFederations(String first, String second, String timeImplementation) {
         for (FederationExecutionInformationSet report : federationReports) {
            Map<String, String> reported = new TreeMap<>();
            for (FederationExecutionInformation info : report) {
               reported.put(info.federationExecutionName, info.logicalTimeImplementationName);
            }
            if (reported.size() == 2
                  && timeImplementation.equals(reported.get(first))
                  && timeImplementation.equals(reported.get(second))) {
               return true;
            }
         }
         return false;
      }

      private boolean hasMembers(
            String federation, String firstName, String secondName, String federateType) {
         for (MemberReport report : memberReports) {
            if (!federation.equals(report.federation)) continue;
            Map<String, String> reported = new TreeMap<>();
            for (FederationExecutionMemberInformation info : report.members) {
               reported.put(info.federateName, info.federateType);
            }
            if (reported.size() == 2
                  && federateType.equals(reported.get(firstName))
                  && federateType.equals(reported.get(secondName))) {
               return true;
            }
         }
         return false;
      }

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
               } else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length == 4) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length >= 6) {
                  reflections.add(new Reflection(arguments));
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  receiveOrderInteractions.add(new Interaction(arguments));
                  callbackOrder.add("receiveInteraction");
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  timestampedInteractions.add(new TimedInteraction(arguments));
                  callbackOrder.add("receiveInteraction");
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("timeAdvanceGrant");
               } else if ("reportFederationExecutions".equals(name)
                     && arguments != null && arguments.length == 1) {
                  federationReports.add(
                     ((FederationExecutionInformationSet) arguments[0]).clone());
               } else if ("reportFederationExecutionMembers".equals(name)
                     && arguments != null && arguments.length == 2) {
                  memberReports.add(new MemberReport((String) arguments[0],
                     ((FederationExecutionMemberInformationSet) arguments[1]).clone()));
               } else if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK multi-execution callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }

   private static final class MemberReport {
      private final String federation;
      private final FederationExecutionMemberInformationSet members;

      private MemberReport(String federation, FederationExecutionMemberInformationSet members) {
         this.federation = federation;
         this.members = members;
      }
   }
}
