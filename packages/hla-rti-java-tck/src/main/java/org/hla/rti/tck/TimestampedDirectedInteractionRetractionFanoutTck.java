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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.TimeUnit;
import java.util.function.BooleanSupplier;

/** Retracts both queued directed copies, then verifies a later fanout and terminal handle boundary. */
final class TimestampedDirectedInteractionRetractionFanoutTck {
   private TimestampedDirectedInteractionRetractionFanoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder publisherRec = new Recorder();
      Recorder firstRec = new Recorder();
      Recorder secondRec = new Recorder();
      String federation = "java-tck-directed-retraction-fanout-" + UUID.randomUUID();
      boolean publisherConnected = false, firstConnected = false, secondConnected = false;
      boolean created = false, publisherJoined = false, firstJoined = false, secondJoined = false;
      boolean attributesPublished = false, firstAttributes = false, secondAttributes = false;
      boolean directedPublished = false, firstDirected = false, secondDirected = false;
      boolean firstConstrained = false, secondConstrained = false, publisherRegulating = false;
      try {
         publisher.connect(publisherRec.proxy(), callbackModel); publisherConnected = true;
         first.connect(firstRec.proxy(), callbackModel); firstConnected = true;
         second.connect(secondRec.proxy(), callbackModel); secondConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-retraction-publisher", federation);
         publisherJoined = true;
         first.joinFederationExecution("java-tck-directed-retraction-first", federation); firstJoined = true;
         second.joinFederationExecution("java-tck-directed-retraction-second", federation); secondJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = first.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = second.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         InteractionClassHandle publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle firstInteraction = first.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle secondInteraction = second.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle firstParameter = first.getParameterHandle(firstInteraction, parameterName);
         ParameterHandle secondParameter = second.getParameterHandle(secondInteraction, parameterName);

         AttributeHandleSet publishedAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet firstAttributeSet = JavaTckSupport.attributeSet(first, firstAttribute);
         AttributeHandleSet secondAttributeSet = JavaTckSupport.attributeSet(second, secondAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes); attributesPublished = true;
         first.subscribeObjectClassAttributes(firstClass, firstAttributeSet); firstAttributes = true;
         second.subscribeObjectClassAttributes(secondClass, secondAttributeSet); secondAttributes = true;
         publisher.publishObjectClassDirectedInteractions(publisherClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction)); directedPublished = true;
         first.subscribeObjectClassDirectedInteractions(firstClass,
            JavaTckSupport.interactionSet(first, firstInteraction)); firstDirected = true;
         second.subscribeObjectClassDirectedInteractions(secondClass,
            JavaTckSupport.interactionSet(second, secondInteraction)); secondDirected = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.drain(first); JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRec.discoveries.contains(target) && secondRec.discoveries.contains(target),
            "both directed subscribers did not discover the publisher-held target");
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(first.getTimeFactory().getName())
               && timeFactory.getName().equals(second.getTimeFactory().getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(first.queryLogicalTime())
               && initial.equals(second.queryLogicalTime()), "federates began at different logical times");
         first.enableTimeConstrained(); firstConstrained = true;
         second.enableTimeConstrained(); secondConstrained = true;
         JavaTckSupport.drain(first); JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRec.timeConstrainedEnabled == 1 && secondRec.timeConstrainedEnabled == 1,
            "both recipients must report Time Constrained enable");
         publisher.enableTimeRegulation(epsilon); publisherRegulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRec.timeRegulationEnabled == 1,
            "publisher did not report Time Regulation enable");

         LogicalTime firstMessageTime = offset(initial, epsilon, 6);
         byte[] firstPayload = new byte[] {0x46, 0x41, 0x4E, 0x31};
         byte[] firstTag = new byte[] {0x46, 0x52, 0x31};
         MessageRetractionReturn canceled = send(publisher, publisherInteraction,
            publisherParameter, target, firstPayload, firstTag, firstMessageTime);
         JavaTckSupport.check(canceled != null && canceled.retractionHandleIsValid && canceled.handle != null,
            "first timestamped directed send returned no valid retraction handle");
         JavaTckSupport.check(firstRec.interactions.isEmpty() && secondRec.interactions.isEmpty(),
            "the first timestamped copies were delivered before a recipient advance");
         first.timeAdvanceRequest(firstMessageTime);
         second.timeAdvanceRequest(firstMessageTime);
         publisher.retract(canceled.handle);
         publisher.timeAdvanceRequest(firstMessageTime);
         JavaTckSupport.drain(publisher); JavaTckSupport.drain(first); JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRec.grants.size() == 1 && secondRec.grants.size() == 1
               && firstMessageTime.equals(firstRec.grants.get(0))
               && firstMessageTime.equals(secondRec.grants.get(0)),
            "retraction did not allow both receivers to reach the first message time");
         JavaTckSupport.check(firstRec.interactions.isEmpty() && secondRec.interactions.isEmpty()
               && firstRec.retractions == 0 && secondRec.retractions == 0,
            "retraction leaked a canceled directed copy or callback to either recipient");

         firstRec.callbackOrder.clear(); secondRec.callbackOrder.clear();
         LogicalTime secondMessageTime = offset(initial, epsilon, 8);
         byte[] secondPayload = new byte[] {0x46, 0x41, 0x4E, 0x32};
         byte[] secondTag = new byte[] {0x46, 0x52, 0x32};
         MessageRetractionReturn delivered = send(publisher, publisherInteraction,
            publisherParameter, target, secondPayload, secondTag, secondMessageTime);
         JavaTckSupport.check(delivered != null && delivered.retractionHandleIsValid && delivered.handle != null,
            "second timestamped directed send returned no valid retraction handle");
         first.timeAdvanceRequest(secondMessageTime);
         second.timeAdvanceRequest(secondMessageTime);
         if (!publisherRec.grants.isEmpty()) publisher.timeAdvanceRequest(secondMessageTime);
         JavaTckSupport.drain(publisher); JavaTckSupport.drain(first); JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRec.interactions.size() == 1 && secondRec.interactions.size() == 1,
            "later timestamped directed message did not reach both recipients exactly once");
         checkDelivery(firstRec, first, firstInteraction, firstParameter, target, secondPayload,
            secondTag, producer, secondMessageTime, delivered.handle);
         checkDelivery(secondRec, second, secondInteraction, secondParameter, target, secondPayload,
            secondTag, producer, secondMessageTime, delivered.handle);
         JavaTckSupport.check(firstRec.grants.size() == 2 && secondRec.grants.size() == 2
               && firstMessageTime.equals(firstRec.grants.get(0))
               && secondMessageTime.equals(firstRec.grants.get(1))
               && firstMessageTime.equals(secondRec.grants.get(0))
               && secondMessageTime.equals(secondRec.grants.get(1)),
            "recipients did not receive grants at the expected first and second times");
         expectException(() -> publisher.retract(delivered.handle), "MessageCanNoLongerBeRetracted");
         JavaTckSupport.check(firstRec.retractions == 0 && secondRec.retractions == 0,
            "retracting the delivered handle produced a stale retraction callback");

         first.disableTimeConstrained(); firstConstrained = false;
         second.disableTimeConstrained(); secondConstrained = false;
         publisher.disableTimeRegulation(); publisherRegulating = false;
      } finally {
         if (firstConstrained) try { first.disableTimeConstrained(); } catch (Exception ignored) { }
         if (secondConstrained) try { second.disableTimeConstrained(); } catch (Exception ignored) { }
         if (publisherRegulating && publisherJoined) try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         if (firstDirected && firstJoined) try { first.unsubscribeObjectClassDirectedInteractions(
            first.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         if (secondDirected && secondJoined) try { second.unsubscribeObjectClassDirectedInteractions(
            second.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         if (firstAttributes && firstJoined) unsubscribe(first, objectClassName, attributeName);
         if (secondAttributes && secondJoined) unsubscribe(second, objectClassName, attributeName);
         if (directedPublished && publisherJoined) try { publisher.unpublishObjectClassDirectedInteractions(
            publisher.getObjectClassHandle(objectClassName)); } catch (Exception ignored) { }
         if (attributesPublished && publisherJoined) try {
            publisher.unpublishObjectClassAttributes(publisher.getObjectClassHandle(objectClassName),
               JavaTckSupport.attributeSet(publisher,
                  publisher.getAttributeHandle(publisher.getObjectClassHandle(objectClassName), attributeName)));
         } catch (Exception ignored) { }
         if (firstJoined) try { first.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         if (secondJoined) try { second.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); } catch (Exception ignored) { }
         if (secondConnected) try { second.disconnect(); } catch (Exception ignored) { }
         if (firstConnected) try { first.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   /** A delivered nonconstrained copy is retracted while a constrained copy remains queued. */
   static void runMixed(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador immediate = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder publisherRec = new Recorder();
      Recorder immediateRec = new Recorder();
      Recorder constrainedRec = new Recorder();
      String federation = "java-tck-directed-mixed-retraction-" + UUID.randomUUID();
      boolean publisherConnected = false, immediateConnected = false, constrainedConnected = false;
      boolean created = false, publisherJoined = false, immediateJoined = false, constrainedJoined = false;
      boolean constrainedTimeEnabled = false, publisherTimeEnabled = false;
      try {
         publisher.connect(publisherRec.proxy(), callbackModel); publisherConnected = true;
         immediate.connect(immediateRec.proxy(), callbackModel); immediateConnected = true;
         constrained.connect(constrainedRec.proxy(), callbackModel); constrainedConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "directed-mixed-retraction-publisher", federation); publisherJoined = true;
         immediate.joinFederationExecution("directed-mixed-retraction-immediate", federation);
         immediateJoined = true;
         constrained.joinFederationExecution("directed-mixed-retraction-constrained", federation);
         constrainedJoined = true;

         ObjectClassHandle objectClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle immediateClass = immediate.getObjectClassHandle(objectClassName);
         ObjectClassHandle constrainedClass = constrained.getObjectClassHandle(objectClassName);
         AttributeHandle marker = publisher.getAttributeHandle(objectClass, attributeName);
         InteractionClassHandle interaction = publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle immediateInteraction = immediate.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle constrainedInteraction = constrained.getInteractionClassHandle(interactionClassName);
         ParameterHandle parameter = publisher.getParameterHandle(interaction, parameterName);
         ParameterHandle immediateParameter = immediate.getParameterHandle(
            immediateInteraction, parameterName);
         ParameterHandle constrainedParameter = constrained.getParameterHandle(
            constrainedInteraction, parameterName);
         RTIambassador[] members = {publisher, immediate, constrained};
         ObjectClassHandle[] classes = {objectClass, immediateClass, constrainedClass};
         InteractionClassHandle[] interactions = {interaction, immediateInteraction,
            constrainedInteraction};
         ParameterHandle[] parameters = {parameter, immediateParameter, constrainedParameter};
         for (int i = 0; i < members.length; ++i) {
            JavaTckSupport.check(objectClassName.equals(members[i].getObjectClassName(classes[i]))
                  && interactionClassName.equals(
                     members[i].getInteractionClassName(interactions[i]))
                  && parameterName.equals(members[i].getParameterName(
                     interactions[i], parameters[i])),
               "directed mixed-fanout FOM names did not round-trip locally");
         }
         publisher.publishObjectClassAttributes(objectClass,
            JavaTckSupport.attributeSet(publisher, marker));
         immediate.subscribeObjectClassAttributes(immediateClass,
            JavaTckSupport.attributeSet(immediate,
               immediate.getAttributeHandle(immediateClass, attributeName)));
         constrained.subscribeObjectClassAttributes(constrainedClass,
            JavaTckSupport.attributeSet(constrained,
               constrained.getAttributeHandle(constrainedClass, attributeName)));
         publisher.publishObjectClassDirectedInteractions(objectClass,
            JavaTckSupport.interactionSet(publisher, interaction));
         publisher.changeInteractionOrderType(interaction, OrderType.TIMESTAMP);
         immediate.subscribeObjectClassDirectedInteractionsUniversally(immediateClass,
            JavaTckSupport.interactionSet(immediate, immediateInteraction));
         constrained.subscribeObjectClassDirectedInteractionsUniversally(constrainedClass,
            JavaTckSupport.interactionSet(constrained, constrainedInteraction));
         ObjectInstanceHandle target = publisher.registerObjectInstance(objectClass);
         String targetName = publisher.getObjectInstanceName(target);
         JavaTckSupport.check(targetName != null && !targetName.isEmpty(),
            "mixed-fanout registration returned no standard object name");
         awaitBoth(immediate, constrained, callbackModel,
            () -> immediateRec.discoveryNames.contains(targetName)
               && constrainedRec.discoveryNames.contains(targetName),
            "both mixed-fanout recipients did not discover the target");
         ObjectInstanceHandle immediateTarget = immediate.getObjectInstanceHandle(targetName);
         ObjectInstanceHandle constrainedTarget = constrained.getObjectInstanceHandle(targetName);
         String producerName = publisher.getFederateName(producer);
         FederateHandle immediateProducer = immediate.getFederateHandle(producerName);
         JavaTckSupport.check(immediateRec.discoveries.size() == 1
               && immediateTarget.equals(immediateRec.discoveries.get(0))
               && constrainedRec.discoveries.size() == 1
               && constrainedTarget.equals(constrainedRec.discoveries.get(0)),
            "mixed-fanout discovery did not preserve receiver-local object handles");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> constrainedTimeFactory = constrained.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(constrainedTimeFactory.getName()),
            "directed mixed-fanout members selected different time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime constrainedInitial = constrained.queryLogicalTime();
         LogicalTimeInterval constrainedEpsilon =
            (LogicalTimeInterval) constrainedTimeFactory.makeEpsilon();
         constrained.enableTimeConstrained(); constrainedTimeEnabled = true;
         awaitEvent(constrained, callbackModel,
            () -> constrainedRec.timeConstrainedEnabled == 1,
            "constrained mixed-fanout recipient did not enable Time Constrained");
         JavaTckSupport.check(constrainedRec.timeConstrainedEnabled == 1,
            "constrained mixed-fanout recipient did not enable Time Constrained");
         publisher.enableTimeRegulation(epsilon); publisherTimeEnabled = true;
         awaitEvent(publisher, callbackModel,
            () -> publisherRec.timeRegulationEnabled == 1,
            "mixed-fanout publisher did not enable Time Regulation");
         JavaTckSupport.check(publisherRec.timeRegulationEnabled == 1,
            "mixed-fanout publisher did not enable Time Regulation");
         LogicalTime messageTime = offset(initial, epsilon, 2);
         LogicalTime constrainedGrant = offset(constrainedInitial, constrainedEpsilon, 2);
         byte[] payload = new byte[] {0x4d, 0x58};
         byte[] tag = new byte[] {0x52, 0x44, 0x49};
         MessageRetractionReturn sent = send(publisher, interaction, parameter,
            target, payload, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "mixed-fanout directed send returned no valid retraction designator");
         awaitEvent(immediate, callbackModel, () -> immediateRec.interactions.size() == 1,
            "nonconstrained mixed-fanout recipient did not receive the directed copy");
         JavaTckSupport.check(immediateRec.interactions.size() == 1
               && constrainedRec.interactions.isEmpty(),
            "mixed-fanout directed copy was not delivered only to the nonconstrained recipient");
         Interaction delivered = immediateRec.interactions.get(0);
         JavaTckSupport.check(immediateInteraction.equals(delivered.interactionClass)
               && immediateTarget.equals(delivered.object)
               && immediateProducer.equals(delivered.producer)
               && delivered.parameters.size() == 1
               && Arrays.equals(payload, delivered.parameters.get(immediateParameter))
               && Arrays.equals(tag, delivered.tag) && messageTime.equals(delivered.time)
               && delivered.sentOrder == OrderType.TIMESTAMP
               && delivered.receivedOrder == OrderType.RECEIVE
               && sent.handle.equals(delivered.retraction),
            "nonconstrained directed callback changed the payload, order, or retraction metadata");

         publisher.retract(sent.handle);
         awaitEvent(immediate, callbackModel,
            () -> immediateRec.requestedRetractions.size() == 1,
            "delivered directed recipient did not receive Request Retraction");
         JavaTckSupport.check(immediateRec.requestedRetractions.size() == 1
               && sent.handle.equals(immediateRec.requestedRetractions.get(0))
               && immediateRec.callbackOrder.equals(Arrays.asList(
                  "receiveDirectedInteraction", "requestRetraction")),
            "delivered directed recipient did not receive one matching Request Retraction");
         JavaTckSupport.check(constrainedRec.interactions.isEmpty()
               && constrainedRec.requestedRetractions.isEmpty(),
            "pending directed recipient received a stale callback before advancing");
         constrained.timeAdvanceRequest(constrainedGrant);
         publisher.timeAdvanceRequest(messageTime);
         awaitBoth(publisher, constrained, callbackModel,
            () -> publisherRec.grants.size() == 1 && constrainedRec.grants.size() == 1,
            "mixed-fanout post-retraction grants did not arrive");
         pumpQuiet(constrained, callbackModel);
         JavaTckSupport.check(constrainedRec.grants.size() == 1
               && constrainedGrant.equals(constrainedRec.grants.get(0))
               && constrainedRec.interactions.isEmpty()
               && constrainedRec.requestedRetractions.isEmpty(),
            "retracted pending directed copy escaped at its Time Advance Grant");
      } finally {
         if (constrainedTimeEnabled && constrainedJoined)
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         if (publisherTimeEnabled && publisherJoined)
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         if (constrainedJoined) try { constrained.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (immediateJoined) try { immediate.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (constrainedConnected) try { constrained.disconnect(); } catch (Exception ignored) { }
         if (immediateConnected) try { immediate.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   /** A timestamped directed send retains a retraction recipient without any constrained peer. */
   static void runImmediateOnly(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRec = new Recorder();
      Recorder receiverRec = new Recorder();
      String federation = "java-tck-directed-immediate-retraction-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false;
      boolean created = false, publisherJoined = false, receiverJoined = false;
      boolean publisherRegulating = false;
      try {
         publisher.connect(publisherRec.proxy(), callbackModel); publisherConnected = true;
         receiver.connect(receiverRec.proxy(), callbackModel); receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "directed-immediate-retraction-publisher", federation); publisherJoined = true;
         receiver.joinFederationExecution("directed-immediate-retraction-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle objectClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle marker = publisher.getAttributeHandle(objectClass, attributeName);
         AttributeHandle receiverMarker = receiver.getAttributeHandle(receiverClass, attributeName);
         InteractionClassHandle interaction = publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle parameter = publisher.getParameterHandle(interaction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction, parameterName);
         JavaTckSupport.check(objectClass.equals(receiverClass) && marker.equals(receiverMarker)
               && interaction.equals(receiverInteraction) && parameter.equals(receiverParameter),
            "directed immediate-only FOM handles changed across federates");
         publisher.publishObjectClassAttributes(objectClass,
            JavaTckSupport.attributeSet(publisher, marker));
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverMarker));
         publisher.publishObjectClassDirectedInteractions(objectClass,
            JavaTckSupport.interactionSet(publisher, interaction));
         publisher.changeInteractionOrderType(interaction, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassDirectedInteractions(receiverClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         ObjectInstanceHandle target = publisher.registerObjectInstance(objectClass);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRec.discoveries.size() == 1
               && receiverRec.discoveries.contains(target),
            "immediate-only directed recipient did not discover the target once");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime initial = publisher.queryLogicalTime();
         publisher.enableTimeRegulation(epsilon); publisherRegulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRec.timeRegulationEnabled == 1,
            "immediate-only publisher did not enable Time Regulation");
         LogicalTime grantTime = offset(initial, epsilon, 2);
         LogicalTime messageTime = offset(initial, epsilon, 4);
         publisher.timeAdvanceRequest(grantTime);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRec.grants.size() == 1
               && grantTime.equals(publisherRec.grants.get(0)),
            "immediate-only publisher did not receive its requested grant");
         JavaTckSupport.check(receiverRec.timeConstrainedEnabled == 0,
            "immediate-only recipient unexpectedly enabled Time Constrained");

         byte[] payload = new byte[] {0x49, 0x4f};
         byte[] tag = new byte[] {0x52, 0x44, 0x49};
         MessageRetractionReturn sent = send(publisher, interaction, parameter,
            target, payload, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "immediate-only directed send returned no valid retraction designator");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRec.interactions.size() == 1,
            "immediate-only directed recipient did not receive exactly one callback");
         Interaction delivered = receiverRec.interactions.get(0);
         JavaTckSupport.check(interaction.equals(delivered.interactionClass)
               && target.equals(delivered.object) && producer.equals(delivered.producer)
               && delivered.parameters.size() == 1
               && Arrays.equals(payload, delivered.parameters.get(receiverParameter))
               && Arrays.equals(tag, delivered.tag) && messageTime.equals(delivered.time)
               && delivered.sentOrder == OrderType.TIMESTAMP
               && delivered.receivedOrder == OrderType.RECEIVE
               && sent.handle.equals(delivered.retraction),
            "immediate-only directed callback changed payload, order, or retraction metadata");

         publisher.retract(sent.handle);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRec.interactions.size() == 1
               && receiverRec.requestedRetractions.size() == 1
               && sent.handle.equals(receiverRec.requestedRetractions.get(0))
               && receiverRec.callbackOrder.equals(Arrays.asList(
                  "receiveDirectedInteraction", "requestRetraction")),
            "immediate-only directed recipient did not receive one matching Request Retraction");
      } finally {
         if (publisherRegulating && publisherJoined)
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void service(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
      else Thread.yield();
   }

   private static void awaitEvent(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String message) throws Exception {
      long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(10);
      while (System.nanoTime() < deadline && !condition.getAsBoolean())
         service(ambassador, callbackModel);
      JavaTckSupport.check(condition.getAsBoolean(), message);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel callbackModel, BooleanSupplier condition, String message)
         throws Exception {
      long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(10);
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         service(first, callbackModel);
         service(second, callbackModel);
      }
      JavaTckSupport.check(condition.getAsBoolean(), message);
   }

   private static void pumpQuiet(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(100);
      while (System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.sleep(1);
      }
   }

   private static MessageRetractionReturn send(RTIambassador publisher,
         InteractionClassHandle interaction, ParameterHandle parameter,
         ObjectInstanceHandle target, byte[] payload, byte[] tag, LogicalTime time) throws Exception {
      ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload);
      return publisher.sendDirectedInteraction(interaction, target, values, tag, time);
   }

   private static void unsubscribe(RTIambassador rti, String className, String attributeName) {
      try {
         ObjectClassHandle cls = rti.getObjectClassHandle(className);
         AttributeHandle attribute = rti.getAttributeHandle(cls, attributeName);
         rti.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(rti, attribute));
      } catch (Exception ignored) { }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static void checkDelivery(Recorder recorder, RTIambassador inspector,
         InteractionClassHandle interaction, ParameterHandle parameter, ObjectInstanceHandle target,
         byte[] payload, byte[] tag, FederateHandle producer, LogicalTime time,
         MessageRetractionHandle retraction) throws Exception {
      JavaTckSupport.check(recorder.callbackOrder.equals(Arrays.asList(
            "receiveDirectedInteraction", "timeAdvanceGrant")),
         "directed delivery did not precede its recipient's grant");
      Interaction actual = recorder.interactions.get(0);
      JavaTckSupport.check(interaction.equals(actual.interactionClass) && target.equals(actual.object)
            && actual.parameters != null && actual.parameters.size() == 1
            && Arrays.equals(payload, actual.parameters.get(parameter)),
         "directed callback returned the wrong target or parameter");
      JavaTckSupport.check(Arrays.equals(tag, actual.tag) && producer.equals(actual.producer)
            && time.equals(actual.time), "directed callback changed the tag, producer, or timestamp");
      JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP,
         "directed callback returned the wrong order metadata");
      JavaTckSupport.check(retraction.equals(actual.retraction) && actual.transportation != null
            && !inspector.getTransportationTypeName(actual.transportation).isEmpty(),
         "directed callback omitted retraction or resolvable transportation metadata");
   }

   private static void expectException(JavaTckSupport.CheckedOperation operation, String expected)
         throws Exception {
      try { operation.run(); }
      catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass())
            if (expected.equals(type.getSimpleName())) return;
         throw new AssertionError("retracting delivered message threw " + exception.getClass().getName()
            + " instead of " + expected, exception);
      }
      throw new AssertionError("retracting delivered message did not throw " + expected);
   }

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
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private volatile int retractions;
      private final List<MessageRetractionHandle> requestedRetractions = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<String> discoveryNames = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) ++timeRegulationEnabled;
               else if ("timeConstrainedEnabled".equals(name)) ++timeConstrainedEnabled;
               else if ("discoverObjectInstance".equals(name) && arguments != null && arguments.length >= 4) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
                  discoveryNames.add((String) arguments[2]);
               }
               else if ("receiveDirectedInteraction".equals(name) && arguments != null && arguments.length == 10) {
                  Interaction event = new Interaction();
                  event.interactionClass = (InteractionClassHandle) arguments[0];
                  event.object = (ObjectInstanceHandle) arguments[1];
                  event.parameters = ((ParameterHandleValueMap) arguments[2]).clone();
                  event.tag = ((byte[]) arguments[3]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[4];
                  event.producer = (FederateHandle) arguments[5];
                  event.time = (LogicalTime<?, ?>) arguments[6];
                  event.sentOrder = (OrderType) arguments[7];
                  event.receivedOrder = (OrderType) arguments[8];
                  event.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(event); callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]); callbackOrder.add(name);
               } else if ("requestRetraction".equals(name)) {
                  ++retractions;
                  requestedRetractions.add((MessageRetractionHandle) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed retraction fanout callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
