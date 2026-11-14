package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
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

/** Tests attribute-order defaults, ownership reset, and federate-local interaction order preferences. */
final class OrderTypeControlsTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OrderTypeControlsTck() { }

   static void runInheritedAttributeDefaultScope(RtiFactory factory, String fom,
         String timeImplementation, String baseObjectClassName,
         String derivedObjectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-inherited-order-scope-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean peerConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean peerJoined = false;
      boolean receiverJoined = false;
      boolean receiverSubscribed = false;
      ObjectClassHandle receiverBaseClass = null;
      AttributeHandleSet receiverBaseAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         peer.connect(peerRecorder.proxy(), callbackModel);
         peerConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-inherited-order-publisher", federation);
         publisherJoined = true;
         FederateHandle peerHandle = peer.joinFederationExecution(
            "java-tck-inherited-order-peer", federation);
         peerJoined = true;
         receiver.joinFederationExecution("java-tck-inherited-order-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherBaseClass =
            publisher.getObjectClassHandle(baseObjectClassName);
         ObjectClassHandle publisherDerivedClass =
            publisher.getObjectClassHandle(derivedObjectClassName);
         ObjectClassHandle peerBaseClass = peer.getObjectClassHandle(baseObjectClassName);
         receiverBaseClass = receiver.getObjectClassHandle(baseObjectClassName);
         AttributeHandle publisherBaseAttribute =
            publisher.getAttributeHandle(publisherBaseClass, attributeName);
         AttributeHandle publisherDerivedAttribute =
            publisher.getAttributeHandle(publisherDerivedClass, attributeName);
         AttributeHandle peerBaseAttribute = peer.getAttributeHandle(peerBaseClass, attributeName);
         AttributeHandle receiverBaseAttribute =
            receiver.getAttributeHandle(receiverBaseClass, attributeName);
         JavaTckSupport.check(publisherBaseClass != null && publisherDerivedClass != null
               && peerBaseClass != null && receiverBaseClass != null
               && publisherBaseAttribute != null && publisherDerivedAttribute != null
               && peerBaseAttribute != null && receiverBaseAttribute != null
               && publisherBaseAttribute.equals(publisherDerivedAttribute)
               && publisherBaseAttribute.equals(peerBaseAttribute)
               && publisherBaseAttribute.equals(receiverBaseAttribute),
            "adapter FOM did not resolve a shared inherited attribute across base and derived classes");

         AttributeHandleSet publisherBaseAttributes =
            JavaTckSupport.attributeSet(publisher, publisherBaseAttribute);
         AttributeHandleSet publisherDerivedAttributes =
            JavaTckSupport.attributeSet(publisher, publisherDerivedAttribute);
         AttributeHandleSet peerBaseAttributes =
            JavaTckSupport.attributeSet(peer, peerBaseAttribute);
         receiverBaseAttributes =
            JavaTckSupport.attributeSet(receiver, receiverBaseAttribute);
         publisher.publishObjectClassAttributes(publisherBaseClass, publisherBaseAttributes);
         publisher.publishObjectClassAttributes(
            publisherDerivedClass, publisherDerivedAttributes);
         peer.publishObjectClassAttributes(peerBaseClass, peerBaseAttributes);
         receiver.subscribeObjectClassAttributes(receiverBaseClass, receiverBaseAttributes);
         receiverSubscribed = true;

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> peerTimeFactory = peer.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(peerTimeFactory.getName()),
            "inherited-order federates selected different logical-time factories");
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         publisher.enableTimeRegulation(publisherEpsilon);
         await(publisher, callbackModel, () -> publisherRecorder.timeRegulationEnabled,
            "inherited-order publisher time-regulation callback");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime peerInitial = peer.queryLogicalTime();

         ObjectInstanceHandle existingReceiveObject =
            publisher.registerObjectInstance(publisherBaseClass);
         await(receiver, callbackModel,
            () -> receiverRecorder.discoveries.contains(existingReceiveObject),
            "inherited-order baseline object discovery");
         sendAndObserveInherited(publisher, receiver, receiverRecorder, existingReceiveObject,
            publisherBaseAttribute, receiverBaseAttribute, publisherHandle, publisherInitial,
            publisherEpsilon, 5, (byte) 0x11, new byte[] {(byte) 0xa1}, false,
            callbackModel, "inherited-order FDD-default baseline");

         publisher.changeDefaultAttributeOrderType(
            publisherDerivedClass, publisherDerivedAttributes, OrderType.TIMESTAMP);
         ObjectInstanceHandle futureBaseTimestampObject =
            publisher.registerObjectInstance(publisherBaseClass);
         await(receiver, callbackModel,
            () -> receiverRecorder.discoveries.contains(futureBaseTimestampObject),
            "inherited-order future base-instance discovery");
         sendAndObserveInherited(publisher, receiver, receiverRecorder,
            futureBaseTimestampObject, publisherBaseAttribute, receiverBaseAttribute,
            publisherHandle, publisherInitial, publisherEpsilon, 6, (byte) 0x12,
            new byte[] {(byte) 0xa2}, true, callbackModel,
            "derived-handle default change on a future base instance");
         sendAndObserveInherited(publisher, receiver, receiverRecorder, existingReceiveObject,
            publisherBaseAttribute, receiverBaseAttribute, publisherHandle, publisherInitial,
            publisherEpsilon, 7, (byte) 0x13, new byte[] {(byte) 0xa3}, false,
            callbackModel, "existing base instance after derived-handle default change");

         ObjectInstanceHandle peerReceiveObject = peer.registerObjectInstance(peerBaseClass);
         await(receiver, callbackModel, () -> receiverRecorder.discoveries.contains(peerReceiveObject),
            "inherited-order peer-object discovery");
         sendAndObserveInherited(peer, receiver, receiverRecorder, peerReceiveObject,
            peerBaseAttribute, receiverBaseAttribute, peerHandle, peerInitial,
            publisherEpsilon, 8, (byte) 0x14, new byte[] {(byte) 0xa4}, false,
            callbackModel, "another federate's unchanged order default");

         publisher.changeDefaultAttributeOrderType(
            publisherBaseClass, publisherBaseAttributes, OrderType.RECEIVE);
         ObjectInstanceHandle futureDerivedReceiveObject =
            publisher.registerObjectInstance(publisherDerivedClass);
         await(receiver, callbackModel,
            () -> receiverRecorder.discoveries.contains(futureDerivedReceiveObject),
            "inherited-order future derived-instance discovery");
         sendAndObserveInherited(publisher, receiver, receiverRecorder,
            futureDerivedReceiveObject, publisherDerivedAttribute, receiverBaseAttribute,
            publisherHandle, publisherInitial, publisherEpsilon, 9, (byte) 0x15,
            new byte[] {(byte) 0xa5}, false, callbackModel,
            "base-handle default change on a future derived instance");
         sendAndObserveInherited(publisher, receiver, receiverRecorder,
            futureBaseTimestampObject, publisherBaseAttribute, receiverBaseAttribute,
            publisherHandle, publisherInitial, publisherEpsilon, 10, (byte) 0x16,
            new byte[] {(byte) 0xa6}, true, callbackModel,
            "existing timestamp instance after base-handle default change");
      } finally {
         if (receiverJoined && receiverSubscribed && receiverBaseClass != null
               && receiverBaseAttributes != null) {
            try { receiver.unsubscribeObjectClassAttributes(
               receiverBaseClass, receiverBaseAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (peerJoined) {
            try { peer.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (peerConnected) {
            try { peer.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runDefaultOrderAttributeClassHandleValidation(RtiFactory factory, String fom,
         String timeImplementation, String baseObjectClassName,
         String derivedObjectClassName, String derivedOnlyAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-default-order-class-handle-validation-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         member.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-default-order-class-handle", federation);
         joined = true;

         ObjectClassHandle baseClass = member.getObjectClassHandle(baseObjectClassName);
         ObjectClassHandle derivedClass = member.getObjectClassHandle(derivedObjectClassName);
         AttributeHandle derivedOnlyAttribute =
            member.getAttributeHandle(derivedClass, derivedOnlyAttributeName);
         JavaTckSupport.check(baseClass != null && derivedClass != null
               && !baseClass.equals(derivedClass) && derivedOnlyAttribute != null,
            "adapter FOM did not provide the configured base, derived class, and derived-only attribute");
         AttributeHandleSet derivedOnlyAttributes =
            JavaTckSupport.attributeSet(member, derivedOnlyAttribute);

         JavaTckSupport.expectFailure(
            () -> member.changeDefaultAttributeOrderType(
               baseClass, derivedOnlyAttributes, OrderType.TIMESTAMP),
            "AttributeNotDefined",
            "changing an order default for a derived-only attribute through its base class");
         member.changeDefaultAttributeOrderType(
            derivedClass, derivedOnlyAttributes, OrderType.TIMESTAMP);
      } finally {
         if (joined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { member.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { member.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador scopedPublisher = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      Recorder scopedPublisherRecorder = new Recorder();
      String federation = "java-tck-order-type-controls-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, scopedPublisherConnected = false;
      boolean created = false, publisherJoined = false, receiverJoined = false, scopedPublisherJoined = false;
      boolean publisherAttributesPublished = false, receiverAttributesSubscribed = false;
      boolean scopedPublisherAttributesPublished = false, scopedPublisherAttributesSubscribed = false;
      boolean publisherInteractionPublished = false, scopedPublisherInteractionPublished = false;
      boolean receiverInteractionSubscribed = false, receiverTimeConstrained = false;
      boolean publisherTimeRegulating = false, scopedPublisherTimeRegulating = false;
      ObjectClassHandle publisherClass = null, receiverClass = null, scopedPublisherClass = null;
      AttributeHandleSet published = null, subscribed = null, scopedPublished = null;
      InteractionClassHandle publisherInteraction = null, receiverInteraction = null;
      InteractionClassHandle scopedPublisherInteraction = null;
      RegionHandle publisherRegion = null, receiverRegion = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         scopedPublisher.connect(scopedPublisherRecorder.proxy(), callbackModel);
         scopedPublisherConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-order-controls-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-order-controls-receiver", federation);
         receiverJoined = true;
         FederateHandle scopedProducer = scopedPublisher.joinFederationExecution(
            "java-tck-order-controls-scoped-publisher", federation);
         scopedPublisherJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         scopedPublisherClass = scopedPublisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         AttributeHandle scopedPublisherAttribute = scopedPublisher.getAttributeHandle(
            scopedPublisherClass, attributeName);
         publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         scopedPublisherInteraction =
            scopedPublisher.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         ParameterHandle scopedPublisherParameter = scopedPublisher.getParameterHandle(
            scopedPublisherInteraction, parameterName);
         DimensionHandle publisherDimension = publisher.getDimensionHandle(dimensionName);
         DimensionHandle receiverDimension = receiver.getDimensionHandle(dimensionName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && scopedPublisherClass != null && publisherAttribute != null
               && receiverAttribute != null && scopedPublisherAttribute != null
               && publisherInteraction != null && receiverInteraction != null
               && scopedPublisherInteraction != null && publisherParameter != null
               && receiverParameter != null && scopedPublisherParameter != null
               && publisherDimension != null && receiverDimension != null
               && publisher.getDimensionUpperBound(publisherDimension) >= 10
               && receiver.getDimensionUpperBound(receiverDimension) >= 10,
            "adapter DDM FOM did not resolve the order-control classes, handles, and dimension range");

         published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         subscribed = receiver.getAttributeHandleSetFactory().create();
         subscribed.add(receiverAttribute);
         scopedPublished = scopedPublisher.getAttributeHandleSetFactory().create();
         scopedPublished.add(scopedPublisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         publisherAttributesPublished = true;
         scopedPublisher.publishObjectClassAttributes(scopedPublisherClass, scopedPublished);
         scopedPublisherAttributesPublished = true;
         receiver.subscribeObjectClassAttributes(receiverClass, subscribed);
         receiverAttributesSubscribed = true;
         scopedPublisher.subscribeObjectClassAttributes(scopedPublisherClass, scopedPublished);
         scopedPublisherAttributesSubscribed = true;
         publisher.publishInteractionClass(publisherInteraction);
         publisherInteractionPublished = true;
         scopedPublisher.publishInteractionClass(scopedPublisherInteraction);
         scopedPublisherInteractionPublished = true;
         publisherRegion = createRegion(publisher, publisherDimension, 0, 6);
         receiverRegion = createRegion(receiver, receiverDimension, 5, 10);
         receiver.subscribeInteractionClassWithRegions(
            receiverInteraction, regionSet(receiver, receiverRegion));
         receiverInteractionSubscribed = true;

         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.RECEIVE);
         scopedPublisher.changeDefaultAttributeOrderType(
            scopedPublisherClass, scopedPublished, OrderType.RECEIVE);
         ObjectInstanceHandle receiveOrderObject = publisher.registerObjectInstance(publisherClass);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.TIMESTAMP);
         ObjectInstanceHandle defaultTimestampObject = publisher.registerObjectInstance(publisherClass);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.RECEIVE);
         ObjectInstanceHandle explicitTimestampObject = publisher.registerObjectInstance(publisherClass);
         AttributeHandleSet instanceAttribute = publisher.getAttributeHandleSetFactory().create();
         instanceAttribute.add(publisherAttribute);
         publisher.changeAttributeOrderType(
            explicitTimestampObject, instanceAttribute, OrderType.TIMESTAMP);
         await(scopedPublisher, callbackModel,
            () -> scopedPublisherRecorder.discoveries.contains(explicitTimestampObject),
            "scoped publisher discovery of the ownership-transfer object");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.discoveries.contains(receiveOrderObject)
               && receiverRecorder.discoveries.contains(defaultTimestampObject)
               && receiverRecorder.discoveries.contains(explicitTimestampObject),
            "receiver did not discover all objects created under the successive order defaults");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> scopedPublisherFactory = scopedPublisher.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName())
               && publisherFactory.getName().equals(scopedPublisherFactory.getName()),
            "order-control federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);
         LogicalTimeInterval scopedLookahead = intervalOffset(epsilon, 20);
         receiver.enableTimeConstrained();
         receiverTimeConstrained = true;
         JavaTckSupport.drain(receiver);
         receiver.enableAsynchronousDelivery();
         publisher.enableTimeRegulation(lookahead);
         publisherTimeRegulating = true;
         JavaTckSupport.drain(publisher);
         scopedPublisher.enableTimeRegulation(scopedLookahead);
         scopedPublisherTimeRegulating = true;
         JavaTckSupport.drain(scopedPublisher);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabled
               && publisherRecorder.timeRegulationEnabled
               && scopedPublisherRecorder.timeRegulationEnabled,
            "time-constrained or publisher time-regulation callback was not delivered");

         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime scopedPublisherInitial = scopedPublisher.queryLogicalTime();
         LogicalTime messageTime = offset(publisherInitial, epsilon, 6);
         LogicalTime receiverTarget = offset(receiverInitial, epsilon, 6);
         JavaTckSupport.check(messageTime.equals(receiverTarget),
            "federates did not derive the same logical-time target");
         byte[] receiveValue = new byte[] {0x11};
         byte[] defaultTimestampValue = new byte[] {0x22};
         byte[] explicitTimestampValue = new byte[] {0x33};
         byte[] attributeTag = new byte[] {0x4f, 0x52, 0x44};
         publisher.updateAttributeValues(receiveOrderObject,
            oneAttribute(publisher, publisherAttribute, receiveValue), attributeTag);
         MessageRetractionReturn defaultRetraction = publisher.updateAttributeValues(
            defaultTimestampObject,
            oneAttribute(publisher, publisherAttribute, defaultTimestampValue),
            attributeTag, messageTime);
         MessageRetractionReturn explicitRetraction = publisher.updateAttributeValues(
            explicitTimestampObject,
            oneAttribute(publisher, publisherAttribute, explicitTimestampValue),
            attributeTag, messageTime);
         JavaTckSupport.check(valid(defaultRetraction) && valid(explicitRetraction),
            "timestamp-ordered attribute updates did not return valid retraction handles");

         receiver.timeAdvanceRequest(receiverTarget);
         publisher.timeAdvanceRequest(messageTime);
         drainAll(publisher, receiver);
         JavaTckSupport.check(receiverRecorder.receiveOrderReflections.size() == 1
               && receiverRecorder.timestampedReflections.size() == 2
               && receiverRecorder.timeAdvanceGrantTimes.size() == 1
               && messageTime.equals(receiverRecorder.timeAdvanceGrantTimes.get(0)),
            "attribute-order defaults did not produce one receive-order and two timestamped reflections");
         assertReceiveOrder(receiverRecorder.receiveOrderReflections.get(0),
            receiveOrderObject, receiverAttribute, receiveValue, attributeTag, producer,
            receiver, "receive-order default");
         assertTimestamped(receiverRecorder.timestampedReflections,
            defaultTimestampObject, explicitTimestampObject, receiverAttribute,
            defaultTimestampValue, explicitTimestampValue, attributeTag, producer,
            messageTime, defaultRetraction.handle, explicitRetraction.handle,
            receiver, "timestamp-order defaults and instance override");

         byte[] transferTag = new byte[] {0x4f, 0x57, 0x4e};
         scopedPublisher.attributeOwnershipAcquisition(
            explicitTimestampObject, scopedPublished, transferTag);
         publisher.negotiatedAttributeOwnershipDivestiture(
            explicitTimestampObject, published, transferTag);
         await(publisher, callbackModel,
            () -> !publisherRecorder.divestitureConfirmations.isEmpty(),
            "attribute-order divestiture-confirmation callback");
         OwnershipEvent divestiture = publisherRecorder.divestitureConfirmations.get(0);
         JavaTckSupport.check(explicitTimestampObject.equals(divestiture.object)
               && divestiture.attributes.size() == 1
               && divestiture.attributes.contains(publisherAttribute)
               && Arrays.equals(transferTag, divestiture.tag),
            "attribute-order divestiture confirmation returned the wrong transfer");
         byte[] confirmationTag = new byte[] {0x43, 0x46, 0x4d};
         publisher.confirmDivestiture(explicitTimestampObject, published, confirmationTag);
         await(scopedPublisher, callbackModel,
            () -> !scopedPublisherRecorder.acquisitions.isEmpty(),
            "attribute-order ownership-acquisition callback");
         OwnershipEvent acquisition = scopedPublisherRecorder.acquisitions.get(0);
         JavaTckSupport.check(explicitTimestampObject.equals(acquisition.object)
               && acquisition.attributes.size() == 1
               && acquisition.attributes.contains(scopedPublisherAttribute)
               && Arrays.equals(confirmationTag, acquisition.tag)
               && !publisher.isAttributeOwnedByFederate(explicitTimestampObject, publisherAttribute)
               && scopedPublisher.isAttributeOwnedByFederate(
                  explicitTimestampObject, scopedPublisherAttribute),
            "attribute-order ownership transfer did not establish the new owner");

         byte[] transferredValue = new byte[] {0x4e, 0x45, 0x57};
         LogicalTime transferredTime = offset(scopedPublisherInitial, epsilon, 32);
         MessageRetractionReturn transferredRetraction = scopedPublisher.updateAttributeValues(
            explicitTimestampObject,
            oneAttribute(scopedPublisher, scopedPublisherAttribute, transferredValue),
            confirmationTag, transferredTime);
         JavaTckSupport.check(!valid(transferredRetraction),
            "transferred attribute retained the prior owner's TIMESTAMP preference");
         await(receiver, callbackModel,
            () -> receiverRecorder.timestampedReflections.size() >= 3,
            "new-owner receive-order timestamped attribute reflection");
         Reflection transferredReflection = receiverRecorder.timestampedReflections.stream()
            .filter(event -> explicitTimestampObject.equals(event.object)
               && Arrays.equals(confirmationTag, event.tag))
            .findFirst().orElse(null);
         JavaTckSupport.check(transferredReflection != null
               && transferredReflection.values.size() == 1
               && Arrays.equals(transferredValue, transferredReflection.values.get(receiverAttribute))
               && transferredTime.equals(transferredReflection.time)
               && scopedProducer.equals(transferredReflection.producer)
               && transferredReflection.sentOrder == OrderType.RECEIVE
               && transferredReflection.receivedOrder == OrderType.RECEIVE
               && transferredReflection.retraction == null
               && transferredReflection.regionsEmpty,
            "new owner did not use its class-default RECEIVE order after acquisition");
         JavaTckSupport.check(receiverRecorder.timestampedReflections.size() == 3
               && receiverRecorder.receiveOrderReflections.size() == 1,
            "new-owner update used the wrong reflection callback family");

         byte[] parameterValue = new byte[] {0x49, 0x4e, 0x54};
         byte[] fddDefaultParameterValue = new byte[] {0x46, 0x44, 0x44};
         byte[] regionalParameterValue = new byte[] {0x52, 0x45, 0x47};
         byte[] interactionTag = new byte[] {0x49, 0x4e, 0x54};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, parameterValue);
         ParameterHandleValueMap defaultParameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         defaultParameters.put(publisherParameter, fddDefaultParameterValue);
         ParameterHandleValueMap regionalParameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         regionalParameters.put(publisherParameter, regionalParameterValue);
         ParameterHandleValueMap scopedParameters =
            scopedPublisher.getParameterHandleValueMapFactory().create(1);
         scopedParameters.put(scopedPublisherParameter, parameterValue);
         LogicalTime defaultInteractionTime = offset(publisherInitial, epsilon, 12);
         MessageRetractionReturn defaultInteractionResult = publisher.sendInteraction(
            publisherInteraction, defaultParameters, interactionTag, defaultInteractionTime);
         JavaTckSupport.check(!valid(defaultInteractionResult),
            "FDD-initialized RECEIVE order unexpectedly returned a retraction handle");

         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         LogicalTime changedTimestampTime = offset(publisherInitial, epsilon, 14);
         MessageRetractionReturn changedTimestampResult = publisher.sendInteraction(
            publisherInteraction, parameters, interactionTag, changedTimestampTime);
         JavaTckSupport.check(valid(changedTimestampResult),
            "changing interaction preference to TIMESTAMP did not affect a later send");
         publisher.retract(changedTimestampResult.handle);

         scopedPublisher.changeInteractionOrderType(scopedPublisherInteraction, OrderType.TIMESTAMP);
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.RECEIVE);
         LogicalTime interactionTime = offset(publisherInitial, epsilon, 16);
         LogicalTime scopedInteractionTime = offset(scopedPublisherInitial, epsilon, 30);
         MessageRetractionReturn scopedInteractionResult = scopedPublisher.sendInteraction(
            scopedPublisherInteraction, scopedParameters, interactionTag, scopedInteractionTime);
         JavaTckSupport.check(valid(scopedInteractionResult),
            "another publisher's interaction-order change altered this federate's timestamp preference");
         scopedPublisher.retract(scopedInteractionResult.handle);

         MessageRetractionReturn regionalInteractionResult = publisher.sendInteractionWithRegions(
            publisherInteraction, regionalParameters, regionSet(publisher, publisherRegion),
            interactionTag, interactionTime);
         JavaTckSupport.check(!valid(regionalInteractionResult),
            "receive-order interaction preference returned a TSO handle for Send Interaction with Regions");
         MessageRetractionReturn interactionResult = publisher.sendInteraction(
            publisherInteraction, parameters, interactionTag, interactionTime);
         JavaTckSupport.check(!valid(interactionResult),
            "receive-order interaction unexpectedly returned a retraction handle");
         await(receiver, callbackModel,
            () -> receiverRecorder.timestampedInteractions.size() >= 3,
            "order-control receive-order interactions");
         drainAll(publisher, receiver, scopedPublisher);
         JavaTckSupport.check(receiverRecorder.timestampedInteractions.size() == 3
               && receiverRecorder.receiveOrderInteractions.isEmpty(),
            "receive-order interactions used the wrong callback overload");
         assertTimestampedReceiveOrderInteraction(receiverRecorder.timestampedInteractions,
            receiverInteraction, receiverParameter, fddDefaultParameterValue, interactionTag,
            producer, defaultInteractionTime, receiver, true, "FDD-default interaction");
         assertTimestampedReceiveOrderInteraction(receiverRecorder.timestampedInteractions,
            receiverInteraction, receiverParameter, regionalParameterValue, interactionTag,
            producer, interactionTime, receiver, null, "regional receive-order interaction");
         assertTimestampedReceiveOrderInteraction(receiverRecorder.timestampedInteractions,
            receiverInteraction, receiverParameter, parameterValue, interactionTag,
            producer, interactionTime, receiver, true, "regular receive-order interaction");
      } finally {
         if (receiverTimeConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherTimeRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (scopedPublisherTimeRegulating && scopedPublisherJoined) {
            try { scopedPublisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (receiverInteractionSubscribed && receiverJoined && receiverInteraction != null
               && receiverRegion != null) {
            try { receiver.unsubscribeInteractionClassWithRegions(
               receiverInteraction, regionSet(receiver, receiverRegion)); }
            catch (Exception ignored) { }
         }
         if (publisherRegion != null && publisherJoined) {
            try { publisher.deleteRegion(publisherRegion); } catch (Exception ignored) { }
         }
         if (receiverRegion != null && receiverJoined) {
            try { receiver.deleteRegion(receiverRegion); } catch (Exception ignored) { }
         }
         if (publisherInteractionPublished && publisherJoined && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (scopedPublisherInteractionPublished && scopedPublisherJoined
               && scopedPublisherInteraction != null) {
            try { scopedPublisher.unpublishInteractionClass(scopedPublisherInteraction); }
            catch (Exception ignored) { }
         }
         if (receiverAttributesSubscribed && receiverJoined && receiverClass != null
               && subscribed != null) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, subscribed); }
            catch (Exception ignored) { }
         }
         if (scopedPublisherAttributesSubscribed && scopedPublisherJoined
               && scopedPublisherClass != null && scopedPublished != null) {
            try { scopedPublisher.unsubscribeObjectClassAttributes(
               scopedPublisherClass, scopedPublished); }
            catch (Exception ignored) { }
         }
         if (publisherAttributesPublished && publisherJoined && publisherClass != null
               && published != null) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, published); }
            catch (Exception ignored) { }
         }
         if (scopedPublisherAttributesPublished && scopedPublisherJoined
               && scopedPublisherClass != null && scopedPublished != null) {
            try { scopedPublisher.unpublishObjectClassAttributes(
               scopedPublisherClass, scopedPublished); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (scopedPublisherJoined) {
            try { scopedPublisher.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
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
         if (scopedPublisherConnected) {
            try { scopedPublisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static AttributeHandleValueMap oneAttribute(RTIambassador ambassador,
         AttributeHandle attribute, byte[] value) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value);
      return values;
   }

   private static RegionHandle createRegion(RTIambassador ambassador,
         DimensionHandle dimension, long lower, long upper) throws Exception {
      DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
      dimensions.add(dimension);
      RegionHandle region = ambassador.createRegion(dimensions);
      ambassador.setRangeBounds(region, dimension,
         new hla.rti1516_2025.RangeBounds(lower, upper));
      ambassador.commitRegionModifications(regionSet(ambassador, region));
      return region;
   }

   private static RegionHandleSet regionSet(RTIambassador ambassador, RegionHandle... regions)
         throws Exception {
      RegionHandleSet result = ambassador.getRegionHandleSetFactory().create();
      for (RegionHandle region : regions) result.add(region);
      return result;
   }

   private static void sendAndObserveInherited(RTIambassador producer,
         RTIambassador receiver, Recorder receiverRecorder, ObjectInstanceHandle object,
         AttributeHandle producerAttribute, AttributeHandle receiverAttribute,
         FederateHandle producerHandle, LogicalTime initial, LogicalTimeInterval epsilon,
         int timeStep, byte value, byte[] tag, boolean timestampOrderExpected,
         CallbackModel callbackModel, String description) throws Exception {
      int receiveOrderBefore = receiverRecorder.receiveOrderReflections.size();
      int timestampedBefore = receiverRecorder.timestampedReflections.size();
      LogicalTime updateTime = offset(initial, epsilon, timeStep);
      MessageRetractionReturn result = null;
      if (timestampOrderExpected) {
         result = producer.updateAttributeValues(object,
            oneAttribute(producer, producerAttribute, new byte[] {value}), tag, updateTime);
         JavaTckSupport.check(valid(result),
            description + " did not return a valid timestamped retraction handle");
      } else {
         producer.updateAttributeValues(object,
            oneAttribute(producer, producerAttribute, new byte[] {value}), tag);
      }
      await(receiver, callbackModel,
         () -> receiverRecorder.receiveOrderReflections.size() > receiveOrderBefore
            || receiverRecorder.timestampedReflections.size() > timestampedBefore,
         description + " reflection");
      drainIfEvoked(receiver, callbackModel);

      if (timestampOrderExpected) {
         JavaTckSupport.check(receiverRecorder.receiveOrderReflections.size() == receiveOrderBefore
               && receiverRecorder.timestampedReflections.size() == timestampedBefore + 1,
            description + " used a receive-order callback or produced duplicate reflections");
         Reflection event = receiverRecorder.timestampedReflections.get(timestampedBefore);
         JavaTckSupport.check(object.equals(event.object) && event.values.size() == 1
               && Arrays.equals(new byte[] {value}, event.values.get(receiverAttribute))
               && Arrays.equals(tag, event.tag) && updateTime.equals(event.time)
               && producerHandle.equals(event.producer)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == OrderType.RECEIVE
               && event.retraction != null && event.regionsEmpty,
            description + " returned incorrect timestamped reflection metadata");
      } else {
         JavaTckSupport.check(receiverRecorder.receiveOrderReflections.size()
               == receiveOrderBefore + 1
               && receiverRecorder.timestampedReflections.size() == timestampedBefore,
            description + " used a timestamped callback or produced duplicate reflections");
         Reflection event = receiverRecorder.receiveOrderReflections.get(receiveOrderBefore);
         JavaTckSupport.check(object.equals(event.object) && event.values.size() == 1
               && Arrays.equals(new byte[] {value}, event.values.get(receiverAttribute))
               && Arrays.equals(tag, event.tag) && producerHandle.equals(event.producer),
            description + " returned incorrect receive-order reflection metadata");
      }
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

   private static void drainIfEvoked(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static boolean valid(MessageRetractionReturn result) {
      return result != null && result.retractionHandleIsValid && result.handle != null;
   }

   private static void drainAll(RTIambassador... ambassadors) throws Exception {
      for (int i = 0; i < 64; ++i) {
         for (RTIambassador ambassador : ambassadors) ambassador.evokeCallback(0.0);
      }
   }

   private static void assertReceiveOrder(Reflection event, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag, FederateHandle producer,
         RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(object.equals(event.object) && event.values.size() == 1
            && Arrays.equals(value, event.values.get(attribute))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty()
            && event.regionsEmpty,
         description + " did not preserve object, payload, or receive-order callback metadata");
   }

   private static void assertTimestamped(List<Reflection> events,
         ObjectInstanceHandle defaultObject, ObjectInstanceHandle explicitObject,
         AttributeHandle attribute, byte[] defaultValue, byte[] explicitValue, byte[] tag,
         FederateHandle producer, LogicalTime time,
         MessageRetractionHandle defaultRetraction,
         MessageRetractionHandle explicitRetraction, RTIambassador inspector,
         String description) throws Exception {
      JavaTckSupport.check(events.size() == 2 && !events.get(0).object.equals(events.get(1).object),
         description + " collapsed or duplicated the two timestamped instances");
      boolean defaultFirst = defaultObject.equals(events.get(0).object)
         && explicitObject.equals(events.get(1).object);
      boolean explicitFirst = explicitObject.equals(events.get(0).object)
         && defaultObject.equals(events.get(1).object);
      JavaTckSupport.check(defaultFirst || explicitFirst,
         description + " returned an unexpected object instance");
      for (Reflection event : events) {
         byte[] expectedValue = defaultObject.equals(event.object) ? defaultValue : explicitValue;
         MessageRetractionHandle expectedRetraction = defaultObject.equals(event.object)
            ? defaultRetraction : explicitRetraction;
         JavaTckSupport.check(event.values.size() == 1
               && Arrays.equals(expectedValue, event.values.get(attribute))
               && Arrays.equals(tag, event.tag) && time.equals(event.time)
               && producer.equals(event.producer)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == OrderType.TIMESTAMP
               && expectedRetraction.equals(event.retraction)
               && event.transportation != null
               && !inspector.getTransportationTypeName(event.transportation).isEmpty()
               && event.regionsEmpty,
            description + " lost value, tag, time, order, producer, transport, or retraction metadata");
      }
   }

   private static void assertReceiveOrderInteraction(Interaction event,
         InteractionClassHandle interactionClass, ParameterHandle parameter,
         byte[] value, byte[] tag, FederateHandle producer, LogicalTime timestamp,
         RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(interactionClass.equals(event.interactionClass)
            && event.parameters.size() == 1 && Arrays.equals(value, event.parameters.get(parameter))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && timestamp.equals(event.time) && event.sentOrder == OrderType.RECEIVE
            && event.receivedOrder == OrderType.RECEIVE && event.retraction == null
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty()
            && event.regionsEmpty,
         description + " did not preserve receive-order delivery and timestamped callback metadata");
   }

   private static void assertTimestampedReceiveOrderInteraction(List<Interaction> events,
         InteractionClassHandle interactionClass, ParameterHandle parameter, byte[] value,
         byte[] tag, FederateHandle producer, LogicalTime time, RTIambassador inspector,
         Boolean regionsEmpty, String description) throws Exception {
      Interaction event = events.stream()
         .filter(candidate -> candidate.parameters.size() == 1
            && Arrays.equals(value, candidate.parameters.get(parameter)))
         .findFirst().orElse(null);
      JavaTckSupport.check(event != null && interactionClass.equals(event.interactionClass)
            && event.parameters.size() == 1
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && time.equals(event.time) && event.sentOrder == OrderType.RECEIVE
            && event.receivedOrder == OrderType.RECEIVE && event.retraction == null
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty()
            && (regionsEmpty == null || event.regionsEmpty == regionsEmpty),
         description + " did not preserve payload, tag, time, receive order, or callback metadata");
    }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Interaction {
      private InteractionClassHandle interactionClass;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes.clone();
         this.tag = tag.clone();
      }
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> receiveOrderReflections = new CopyOnWriteArrayList<>();
      private final List<Reflection> timestampedReflections = new CopyOnWriteArrayList<>();
      private final List<Interaction> receiveOrderInteractions = new CopyOnWriteArrayList<>();
      private final List<Interaction> timestampedInteractions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> divestitureConfirmations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("requestDivestitureConfirmation".equals(name)
                     && arguments != null && arguments.length == 3) {
                  divestitureConfirmations.add(new OwnershipEvent(
                     (ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new OwnershipEvent(
                     (ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && (arguments.length == 6 || arguments.length == 10)) {
                  Reflection event = new Reflection();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  if (arguments.length == 10) {
                     event.time = (LogicalTime<?, ?>) arguments[6];
                     event.sentOrder = (OrderType) arguments[7];
                     event.receivedOrder = (OrderType) arguments[8];
                     event.retraction = (MessageRetractionHandle) arguments[9];
                     timestampedReflections.add(event);
                  } else {
                     receiveOrderReflections.add(event);
                  }
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && (arguments.length == 6 || arguments.length == 10)) {
                  Interaction event = new Interaction();
                  event.interactionClass = (InteractionClassHandle) arguments[0];
                  event.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  if (arguments.length == 10) {
                     event.time = (LogicalTime<?, ?>) arguments[6];
                     event.sentOrder = (OrderType) arguments[7];
                     event.receivedOrder = (OrderType) arguments[8];
                     event.retraction = (MessageRetractionHandle) arguments[9];
                     timestampedInteractions.add(event);
                  } else {
                     receiveOrderInteractions.add(event);
                  }
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK order type callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
