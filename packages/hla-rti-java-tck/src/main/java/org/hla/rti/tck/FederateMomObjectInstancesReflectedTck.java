package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
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
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies distinct-object reflection accounting in the standard federate MOM. */
final class FederateMomObjectInstancesReflectedTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String SET_TIMING =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomObjectInstancesReflectedTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador monitor = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder publisherRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      Recorder monitorRecorder = new Recorder();
      String federation = "java-tck-federate-mom-object-instances-reflected-" + UUID.randomUUID();
      boolean publisherConnected = false, subjectConnected = false, monitorConnected = false;
      boolean created = false, publisherJoined = false, subjectJoined = false, monitorJoined = false;
      boolean momSubscribed = false, subjectSubscribed = false, applicationPublished = false;
      ObjectClassHandle momClass = null, publisherClass = null, subjectClass = null;
      AttributeHandleSet momAttributes = null, publisherAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         monitor.connect(monitorRecorder.proxy(), callbackModel);
         monitorConnected = true;
         publisher.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "mom-reflected-publisher", "java-tck", federation);
         publisherJoined = true;
         monitor.joinFederationExecution("mom-reflected-monitor", "java-tck", federation);
         monitorJoined = true;

         momClass = monitor.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = monitor.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle countAttribute = monitor.getAttributeHandle(momClass,
            "HLAobjectInstancesReflected");
         TransportationTypeHandle reliable = monitor.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(monitor.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(monitor.getAttributeName(momClass,
                  federateAttribute))
               && "HLAobjectInstancesReflected".equals(monitor.getAttributeName(momClass,
                  countAttribute))
               && "HLAreliable".equals(monitor.getTransportationTypeName(reliable)),
            "standard federate MOM object-instances-reflected lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(monitor, federateAttribute, countAttribute);
         monitor.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         FederateHandle subjectFederate = subject.joinFederationExecution(
            "mom-reflected-subject", "java-tck", federation);
         subjectJoined = true;
         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subjectClass = subject.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         AttributeHandle subjectAttribute = subject.getAttributeHandle(subjectClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(subjectClass)
               && publisherAttribute.equals(subjectAttribute)
               && objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(subject.getObjectClassName(subjectClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass,
                  publisherAttribute))
               && attributeName.equals(subject.getAttributeName(subjectClass, subjectAttribute)),
            "adapter-supplied publisher and subject object handles did not round-trip");
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         applicationPublished = true;
         subject.subscribeObjectClassAttributes(subjectClass,
            JavaTckSupport.attributeSet(subject, subjectAttribute), "");
         subjectSubscribed = true;

         ObjectInstanceHandle firstObject = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle secondObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "application registration returned invalid or duplicate object handles");
         awaitDiscoveries(subject, subjectRecorder, callbackModel, firstObject, secondObject);
         byte[] encodedSubject = encode(subjectFederate);
         await(monitor, callbackModel,
            () -> monitorRecorder.momObject(federateAttribute, encodedSubject) != null,
            "subject federate MOM discovery by monitor");
         ObjectInstanceHandle subjectMomObject = monitorRecorder.momObject(federateAttribute,
            encodedSubject);
         JavaTckSupport.check(subjectMomObject != null
               && momClass.equals(monitor.getKnownObjectClassHandle(subjectMomObject)),
            "monitor found the wrong subject MOM object");
         JavaTckSupport.check(requestCount(monitor, monitorRecorder, callbackModel,
               subjectMomObject, countAttribute, reliable, encoder,
               "initial object-instances-reflected") == 0,
            "HLAobjectInstancesReflected was nonzero before application reflections");

         sendUpdate(publisher, subject, subjectRecorder, callbackModel, firstObject,
            publisherAttribute, subjectAttribute, publisherFederate,
            (byte) 0x51, reliable, encoder, "first object reflection");
         JavaTckSupport.check(requestCount(monitor, monitorRecorder, callbackModel,
               subjectMomObject, countAttribute, reliable, encoder,
               "count after first object reflection") == 1,
            "HLAobjectInstancesReflected did not count the first reflected object");
         sendUpdate(publisher, subject, subjectRecorder, callbackModel, firstObject,
            publisherAttribute, subjectAttribute, publisherFederate,
            (byte) 0x52, reliable, encoder, "repeated reflection for first object");
         JavaTckSupport.check(requestCount(monitor, monitorRecorder, callbackModel,
               subjectMomObject, countAttribute, reliable, encoder,
               "count after repeated first-object reflection") == 1,
            "HLAobjectInstancesReflected counted repeated reflections of one object more than once");
         sendUpdate(publisher, subject, subjectRecorder, callbackModel, secondObject,
            publisherAttribute, subjectAttribute, publisherFederate,
            (byte) 0x53, reliable, encoder, "reflection for second object");
         int directBaseline = requestCount(monitor, monitorRecorder, callbackModel, subjectMomObject,
            countAttribute, reliable, encoder, "direct distinct-object reflection baseline");
         JavaTckSupport.check(directBaseline == 2,
            "HLAobjectInstancesReflected did not count two distinct objects");

         InteractionClassHandle setTiming = monitor.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = monitor.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = monitor.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(monitor.getInteractionClassName(setTiming))
               && "HLAfederate".equals(monitor.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(monitor.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming lookup did not round-trip");
         int periodicReflectionStart = monitorRecorder.reflections.size();
         sendTiming(monitor, setTiming, timingFederate, reportPeriod, subjectFederate, 1, encoder);
         ObjectInstanceHandle thirdObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(thirdObject != null && !thirdObject.equals(firstObject)
               && !thirdObject.equals(secondObject),
            "third application registration returned an invalid or duplicate handle");
         awaitDiscoveries(subject, subjectRecorder, callbackModel, thirdObject);
         sendUpdate(publisher, subject, subjectRecorder, callbackModel, thirdObject,
            publisherAttribute, subjectAttribute, publisherFederate,
            (byte) 0x54, reliable, encoder, "first periodic-phase reflection for third object");
         sendUpdate(publisher, subject, subjectRecorder, callbackModel, thirdObject,
            publisherAttribute, subjectAttribute, publisherFederate,
            (byte) 0x55, reliable, encoder, "repeated periodic-phase reflection for third object");
         await(monitor, callbackModel,
            () -> findCountAtLeast(monitorRecorder, periodicReflectionStart, subjectMomObject,
               countAttribute, encoder, 3) != null,
            "periodic object-instances-reflected value of three");
         Reflection periodic = findCountAtLeast(monitorRecorder, periodicReflectionStart,
            subjectMomObject, countAttribute, encoder, 3);
         JavaTckSupport.check(periodic != null
               && verifyCount(periodic, countAttribute, reliable, encoder,
                  "periodic object-instances-reflected") == 3,
            "periodic object-instances-reflected did not count three distinct objects");
         JavaTckSupport.check(requestCount(monitor, monitorRecorder, callbackModel, subjectMomObject,
               countAttribute, reliable, encoder, "direct count after periodic reflections") == 3,
            "direct object-instances-reflected disagreed with periodic reporting");
         sendTiming(monitor, setTiming, timingFederate, reportPeriod, subjectFederate, 0, encoder);
         JavaTckSupport.check(requestCount(monitor, monitorRecorder, callbackModel, subjectMomObject,
               countAttribute, reliable, encoder, "count after disabling periodic reports") == 3,
            "HLAobjectInstancesReflected changed after periodic reporting was disabled");

         subject.unsubscribeObjectClassAttributes(subjectClass,
            JavaTckSupport.attributeSet(subject, subjectAttribute));
         subjectSubscribed = false;
         monitor.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         applicationPublished = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         monitor.resignFederationExecution(ResignAction.NO_ACTION);
         monitorJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subjectSubscribed && subjectJoined && subjectClass != null) try {
            subject.unsubscribeObjectClassAttributes(subjectClass,
               JavaTckSupport.attributeSet(subject,
                  subject.getAttributeHandle(subjectClass, attributeName)));
         } catch (Exception ignored) { }
         if (momSubscribed && monitorJoined && momClass != null && momAttributes != null) try {
            monitor.unsubscribeObjectClassAttributes(momClass, momAttributes);
         } catch (Exception ignored) { }
         if (applicationPublished && publisherJoined && publisherClass != null
               && publisherAttributes != null) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         if (subjectJoined) resign(subject);
         if (monitorJoined) resign(monitor);
         if (publisherJoined) resign(publisher);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (monitorConnected) disconnect(monitor);
         if (subjectConnected) disconnect(subject);
         if (publisherConnected) disconnect(publisher);
      }
   }

   static void runReflectionCounts(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "direct and periodic reflection-count testing requires the adapter's logical-time implementation");
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-federate-mom-reflection-counts-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean momSubscribed = false, applicationSubscribed = false, applicationPublished = false;
      ObjectClassHandle momClass = null, publisherClass = null, receiverClass = null;
      AttributeHandleSet momAttributes = null, publisherAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         receiver.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle receiverFederate = receiver.joinFederationExecution(
            "mom-reflection-count-receiver", "java-tck", federation);
         receiverJoined = true;

         momClass = receiver.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = receiver.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle reflectedObjectsAttribute = receiver.getAttributeHandle(momClass,
            "HLAobjectInstancesReflected");
         AttributeHandle reflectionsReceivedAttribute = receiver.getAttributeHandle(momClass,
            "HLAreflectionsReceived");
         TransportationTypeHandle reliable = receiver.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(receiver.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(receiver.getAttributeName(momClass,
                  federateAttribute))
               && "HLAobjectInstancesReflected".equals(receiver.getAttributeName(momClass,
                  reflectedObjectsAttribute))
               && "HLAreflectionsReceived".equals(receiver.getAttributeName(momClass,
                  reflectionsReceivedAttribute))
               && "HLAreliable".equals(receiver.getTransportationTypeName(reliable)),
            "standard MOM reflection-count surface did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(receiver, federateAttribute,
            reflectedObjectsAttribute, reflectionsReceivedAttribute);
         receiver.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;
         byte[] encodedReceiver = encode(receiverFederate);
         await(receiver, callbackModel,
            () -> receiverRecorder.momObject(federateAttribute, encodedReceiver) != null,
            "receiver MOM-object discovery for reflection counts");
         ObjectInstanceHandle receiverMomObject = receiverRecorder.momObject(
            federateAttribute, encodedReceiver);
         JavaTckSupport.check(receiverMomObject != null
               && momClass.equals(receiver.getKnownObjectClassHandle(receiverMomObject)),
            "reflection-count query did not identify the receiver's standard MOM object");
         int[] counts = requestReflectionCounts(receiver, receiverRecorder, callbackModel,
            receiverMomObject, reflectedObjectsAttribute, reflectionsReceivedAttribute,
            reliable, encoder, "initial reflection counts");
         JavaTckSupport.check(counts[0] == 0 && counts[1] == 0,
            "reflection counters were nonzero before application updates");

         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "mom-reflection-count-publisher", "java-tck", federation);
         publisherJoined = true;
         publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(receiverClass,
            attributeName);
         JavaTckSupport.check(objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(receiver.getObjectClassName(receiverClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass,
                  publisherAttribute))
               && attributeName.equals(receiver.getAttributeName(receiverClass, receiverAttribute))
               && publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute),
            "adapter-supplied application object and attribute handles did not round-trip");
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         applicationPublished = true;
         receiver.subscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute), "");
         applicationSubscribed = true;
         ObjectInstanceHandle firstObject = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle secondObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "reflection-count registration returned invalid or duplicate object handles");
         awaitDiscoveries(receiver, receiverRecorder, callbackModel, firstObject, secondObject);

         sendUpdate(publisher, receiver, receiverRecorder, callbackModel, firstObject,
            publisherAttribute, receiverAttribute, publisherFederate, (byte) 0x31, reliable, encoder,
            "first receive-order reflection");
         counts = requestReflectionCounts(receiver, receiverRecorder, callbackModel,
            receiverMomObject, reflectedObjectsAttribute, reflectionsReceivedAttribute,
            reliable, encoder, "counts after first reflection");
         JavaTckSupport.check(counts[0] == 1 && counts[1] == 1,
            "first reflection did not increment distinct-object and invocation counts once");
         sendUpdate(publisher, receiver, receiverRecorder, callbackModel, firstObject,
            publisherAttribute, receiverAttribute, publisherFederate, (byte) 0x32, reliable,
            encoder, "repeated reflection of first object");
         counts = requestReflectionCounts(receiver, receiverRecorder, callbackModel,
            receiverMomObject, reflectedObjectsAttribute, reflectionsReceivedAttribute,
            reliable, encoder, "counts after repeated reflection");
         JavaTckSupport.check(counts[0] == 1 && counts[1] == 2,
            "repeated reflection changed distinct-object or invocation counts incorrectly");
         sendUpdate(publisher, receiver, receiverRecorder, callbackModel, secondObject,
            publisherAttribute, receiverAttribute, publisherFederate, (byte) 0x33, reliable,
            encoder, "reflection of second object");
         counts = requestReflectionCounts(receiver, receiverRecorder, callbackModel,
            receiverMomObject, reflectedObjectsAttribute, reflectionsReceivedAttribute,
            reliable, encoder, "counts after second object reflection");
         JavaTckSupport.check(counts[0] == 2 && counts[1] == 3,
            "second object reflection did not distinguish object count from invocation count");

         publisher.changeAttributeOrderType(secondObject, publisherAttributes, OrderType.TIMESTAMP);
         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(receiverTimeFactory.getName()),
            "reflection-count members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverRecorder.timeConstrainedEnabled != null,
            "receiver time-constrained enablement");
         publisher.enableTimeRegulation(epsilon);
         await(publisher, callbackModel,
            () -> publisherRecorder.timeRegulationEnabled != null,
            "publisher time-regulation enablement");
         LogicalTime timestamp = offset(publisher.queryLogicalTime(), epsilon, 2);
         LogicalTime receiverTarget = offset(receiver.queryLogicalTime(), epsilon, 2);
         JavaTckSupport.check(timestamp.equals(receiverTarget),
            "reflection-count members derived different timestamp targets");
         int timedBefore = receiverRecorder.timedReflections.size();
         MessageRetractionReturn retraction = publisher.updateAttributeValues(secondObject,
            singleAttributeUpdate(publisher, publisherAttribute, encoder, (byte) 0x34),
            new byte[0], timestamp);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped reflection-count update returned an invalid retraction handle");
         receiver.timeAdvanceRequest(receiverTarget);
         publisher.timeAdvanceRequest(timestamp);
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherRecorder.timeAdvanceGrant != null
               && receiverRecorder.timeAdvanceGrant != null,
            "timestamped reflection-count time-advance grants");
         await(receiver, callbackModel,
            () -> receiverRecorder.timedReflections.size() > timedBefore,
            "timestamped application reflection");
         JavaTckSupport.check(receiverRecorder.timedReflections.size() == timedBefore + 1
               && timestamp.equals(publisherRecorder.timeAdvanceGrant)
               && receiverTarget.equals(receiverRecorder.timeAdvanceGrant),
            "timestamped reflection produced duplicate callbacks or incorrect time grants");
         TimedReflection timedReflection = receiverRecorder.timedReflections.get(timedBefore);
         JavaTckSupport.check(secondObject.equals(timedReflection.object)
               && timedReflection.values.size() == 1
               && timedReflection.values.containsKey(receiverAttribute)
               && Arrays.equals(encoder.createHLAbyte((byte) 0x34).toByteArray(),
                  timedReflection.values.get(receiverAttribute))
               && timedReflection.tag.length == 0 && timestamp.equals(timedReflection.time)
               && timedReflection.sentOrder == OrderType.TIMESTAMP
               && timedReflection.receivedOrder == OrderType.TIMESTAMP
               && publisherFederate.equals(timedReflection.producer)
               && reliable.equals(timedReflection.transportation)
               && retraction.handle.equals(timedReflection.retraction)
               && (timedReflection.regions == null || timedReflection.regions.isEmpty()),
            "timestamped reflection changed its value, time, order, or delivery metadata");
         counts = requestReflectionCounts(receiver, receiverRecorder, callbackModel,
            receiverMomObject, reflectedObjectsAttribute, reflectionsReceivedAttribute,
            reliable, encoder, "counts after timestamped reflection");
         JavaTckSupport.check(counts[0] == 2 && counts[1] == 4,
            "timestamped reflection did not update object and callback counts as expected");

         InteractionClassHandle setTiming = receiver.getInteractionClassHandle(SET_TIMING);
         ParameterHandle timingFederate = receiver.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle reportPeriod = receiver.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(receiver.getInteractionClassName(setTiming))
               && "HLAfederate".equals(receiver.getParameterName(setTiming, timingFederate))
               && "HLAreportPeriod".equals(receiver.getParameterName(setTiming, reportPeriod)),
            "standard HLAsetTiming handles did not round-trip");
         int periodicStart = receiverRecorder.reflections.size();
         sendTiming(receiver, setTiming, timingFederate, reportPeriod, receiverFederate, 1, encoder);
         await(receiver, callbackModel,
            () -> findTwoCountReflection(receiverRecorder, periodicStart, receiverMomObject,
               reflectedObjectsAttribute, reflectionsReceivedAttribute) != null,
            "periodic reflection-count report");
         Reflection periodic = findTwoCountReflection(receiverRecorder, periodicStart,
            receiverMomObject, reflectedObjectsAttribute, reflectionsReceivedAttribute);
         int[] periodicCounts = decodeReflectionCounts(periodic, reflectedObjectsAttribute,
            reflectionsReceivedAttribute, reliable, encoder, "periodic reflection counts");
         JavaTckSupport.check(periodicCounts[0] == 2 && periodicCounts[1] == 4,
            "periodic reflection report did not preserve both current counters");
         sendTiming(receiver, setTiming, timingFederate, reportPeriod, receiverFederate, 0, encoder);

         receiver.unsubscribeObjectClassAttributes(receiverClass,
            JavaTckSupport.attributeSet(receiver, receiverAttribute));
         applicationSubscribed = false;
         receiver.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         applicationPublished = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (applicationSubscribed && receiverJoined && receiverClass != null) try {
            receiver.unsubscribeObjectClassAttributes(receiverClass,
               JavaTckSupport.attributeSet(receiver,
                  receiver.getAttributeHandle(receiverClass, attributeName)));
         } catch (Exception ignored) { }
         if (momSubscribed && receiverJoined && momClass != null && momAttributes != null) try {
            receiver.unsubscribeObjectClassAttributes(momClass, momAttributes);
         } catch (Exception ignored) { }
         if (applicationPublished && publisherJoined && publisherClass != null
               && publisherAttributes != null) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher, ResignAction.DELETE_OBJECTS);
         if (created) try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static AttributeHandleValueMap singleAttributeUpdate(RTIambassador publisher,
         AttributeHandle attribute, EncoderFactory encoder, byte value) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, encoder.createHLAbyte(value).toByteArray());
      return values;
   }

   private static int[] requestReflectionCounts(RTIambassador receiver, Recorder recorder,
         CallbackModel model, ObjectInstanceHandle momObject, AttributeHandle reflectedObjects,
         AttributeHandle reflectionsReceived, TransportationTypeHandle reliable,
         EncoderFactory encoder, String description) throws Exception {
      int first = recorder.reflections.size();
      receiver.requestAttributeValueUpdate(momObject,
         JavaTckSupport.attributeSet(receiver, reflectedObjects, reflectionsReceived), new byte[0]);
      await(receiver, model,
         () -> findTwoCountReflection(recorder, first, momObject, reflectedObjects,
            reflectionsReceived) != null,
         description);
      Reflection reflection = findTwoCountReflection(recorder, first, momObject, reflectedObjects,
         reflectionsReceived);
      return decodeReflectionCounts(reflection, reflectedObjects, reflectionsReceived,
         reliable, encoder, description);
   }

   private static Reflection findTwoCountReflection(Recorder recorder, int first,
         ObjectInstanceHandle momObject, AttributeHandle reflectedObjects,
         AttributeHandle reflectionsReceived) {
      for (int i = Math.min(first, recorder.reflections.size()); i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (momObject.equals(reflection.object)
               && reflection.values.containsKey(reflectedObjects)
               && reflection.values.containsKey(reflectionsReceived)) return reflection;
      }
      return null;
   }

   private static int[] decodeReflectionCounts(Reflection reflection,
         AttributeHandle reflectedObjects, AttributeHandle reflectionsReceived,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      JavaTckSupport.check(reflection != null && reflection.values.size() == 2
            && reflection.tag.length == 0 && reliable.equals(reflection.transportation)
            && reflection.producer == null && (reflection.regions == null
               || reflection.regions.isEmpty()),
         description + " returned non-standard MOM reflection metadata");
      HLAinteger32BE objectCount = encoder.createHLAinteger32BE();
      HLAinteger32BE reflectionCount = encoder.createHLAinteger32BE();
      objectCount.decode(reflection.values.get(reflectedObjects));
      reflectionCount.decode(reflection.values.get(reflectionsReceived));
      JavaTckSupport.check(objectCount.getValue() >= 0 && reflectionCount.getValue() >= 0,
         description + " returned negative counters");
      return new int[] {objectCount.getValue(), reflectionCount.getValue()};
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.0);
            second.evokeCallback(0.0);
         } else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   static void runReflectionsReceived(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador monitor = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder publisherRecorder = new Recorder();
      Recorder subjectRecorder = new Recorder();
      Recorder monitorRecorder = new Recorder();
      String federation = "java-tck-federate-mom-reflections-received-" + UUID.randomUUID();
      boolean publisherConnected = false, subjectConnected = false, monitorConnected = false;
      boolean created = false, publisherJoined = false, subjectJoined = false, monitorJoined = false;
      boolean momSubscribed = false, subjectSubscribed = false, applicationPublished = false;
      ObjectClassHandle momClass = null, publisherClass = null, subjectClass = null;
      AttributeHandleSet momAttributes = null, publisherAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         monitor.connect(monitorRecorder.proxy(), callbackModel);
         monitorConnected = true;
         publisher.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle publisherFederate = publisher.joinFederationExecution(
            "mom-reflections-received-publisher", "java-tck", federation);
         publisherJoined = true;
         monitor.joinFederationExecution("mom-reflections-received-monitor", "java-tck",
            federation);
         monitorJoined = true;

         momClass = monitor.getObjectClassHandle(MOM_CLASS);
         AttributeHandle federateAttribute = monitor.getAttributeHandle(momClass,
            "HLAfederateHandle");
         AttributeHandle countAttribute = monitor.getAttributeHandle(momClass,
            "HLAreflectionsReceived");
         TransportationTypeHandle reliable = monitor.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(MOM_CLASS.equals(monitor.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(monitor.getAttributeName(momClass,
                  federateAttribute))
               && "HLAreflectionsReceived".equals(monitor.getAttributeName(momClass,
                  countAttribute))
               && "HLAreliable".equals(monitor.getTransportationTypeName(reliable)),
            "standard federate MOM reflections-received lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(monitor, federateAttribute, countAttribute);
         monitor.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         FederateHandle subjectFederate = subject.joinFederationExecution(
            "mom-reflections-received-subject", "java-tck", federation);
         subjectJoined = true;
         publisherClass = publisher.getObjectClassHandle(objectClassName);
         subjectClass = subject.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass,
            attributeName);
         AttributeHandle subjectAttribute = subject.getAttributeHandle(subjectClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(subjectClass)
               && publisherAttribute.equals(subjectAttribute)
               && objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(subject.getObjectClassName(subjectClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass,
                  publisherAttribute))
               && attributeName.equals(subject.getAttributeName(subjectClass, subjectAttribute)),
            "adapter-supplied application object and attribute handles did not round-trip");
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         applicationPublished = true;
         subject.subscribeObjectClassAttributes(subjectClass,
            JavaTckSupport.attributeSet(subject, subjectAttribute), "");
         subjectSubscribed = true;

         ObjectInstanceHandle firstObject = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle secondObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(firstObject != null && secondObject != null
               && !firstObject.equals(secondObject),
            "application registration returned invalid or duplicate object handles");
         awaitDiscoveries(subject, subjectRecorder, callbackModel, firstObject, secondObject);
         byte[] encodedSubject = encode(subjectFederate);
         await(monitor, callbackModel,
            () -> monitorRecorder.momObject(federateAttribute, encodedSubject) != null,
            "subject federate MOM discovery by monitor");
         ObjectInstanceHandle subjectMomObject = monitorRecorder.momObject(federateAttribute,
            encodedSubject);
         JavaTckSupport.check(subjectMomObject != null
               && momClass.equals(monitor.getKnownObjectClassHandle(subjectMomObject)),
            "monitor found the wrong subject MOM object");
         int baseline = requestCount(monitor, monitorRecorder, callbackModel, subjectMomObject,
            countAttribute, reliable, encoder, "baseline reflections-received");
         JavaTckSupport.check(baseline <= Integer.MAX_VALUE - 3,
            "HLAreflectionsReceived baseline cannot safely accommodate three callbacks");

         sendAndCheckCount(publisher, subject, subjectRecorder, monitor, monitorRecorder,
            callbackModel, firstObject, publisherAttribute, subjectAttribute, publisherFederate,
            subjectMomObject, countAttribute, reliable, encoder, baseline + 1,
            (byte) 0x61, "first receive-order reflection");
         sendAndCheckCount(publisher, subject, subjectRecorder, monitor, monitorRecorder,
            callbackModel, firstObject, publisherAttribute, subjectAttribute, publisherFederate,
            subjectMomObject, countAttribute, reliable, encoder, baseline + 2,
            (byte) 0x62, "repeated reflection of the same object");
         sendAndCheckCount(publisher, subject, subjectRecorder, monitor, monitorRecorder,
            callbackModel, secondObject, publisherAttribute, subjectAttribute, publisherFederate,
            subjectMomObject, countAttribute, reliable, encoder, baseline + 3,
            (byte) 0x63, "reflection of a second object");

         InteractionClassHandle setTiming = monitor.getInteractionClassHandle(SET_TIMING);
         ParameterHandle federateParameter = monitor.getParameterHandle(setTiming, "HLAfederate");
         ParameterHandle periodParameter = monitor.getParameterHandle(setTiming, "HLAreportPeriod");
         JavaTckSupport.check(SET_TIMING.equals(monitor.getInteractionClassName(setTiming))
               && "HLAfederate".equals(monitor.getParameterName(setTiming, federateParameter))
               && "HLAreportPeriod".equals(monitor.getParameterName(setTiming, periodParameter)),
            "standard HLAsetTiming lookup did not round-trip");
         int periodicStart = monitorRecorder.reflections.size();
         sendTiming(monitor, setTiming, federateParameter, periodParameter, subjectFederate, 1,
            encoder);
         await(monitor, callbackModel,
            () -> findCountAtLeast(monitorRecorder, periodicStart, subjectMomObject, countAttribute,
               encoder, baseline + 3) != null,
            "periodic HLAreflectionsReceived value");
         Reflection periodic = findCountAtLeast(monitorRecorder, periodicStart, subjectMomObject,
            countAttribute, encoder, baseline + 3);
         JavaTckSupport.check(periodic != null
               && verifyCount(periodic, countAttribute, reliable, encoder,
                  "periodic reflections-received") == baseline + 3,
            "periodic HLAreflectionsReceived did not preserve all three callbacks");
         sendTiming(monitor, setTiming, federateParameter, periodParameter, subjectFederate, 0,
            encoder);
         JavaTckSupport.check(requestCount(monitor, monitorRecorder, callbackModel, subjectMomObject,
               countAttribute, reliable, encoder,
               "direct reflections-received after disabling reports") == baseline + 3,
            "direct HLAreflectionsReceived changed after periodic reporting was disabled");

         subject.unsubscribeObjectClassAttributes(subjectClass,
            JavaTckSupport.attributeSet(subject, subjectAttribute));
         subjectSubscribed = false;
         monitor.unsubscribeObjectClassAttributes(momClass, momAttributes);
         momSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         applicationPublished = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         monitor.resignFederationExecution(ResignAction.NO_ACTION);
         monitorJoined = false;
         publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subjectSubscribed && subjectJoined && subjectClass != null) try {
            subject.unsubscribeObjectClassAttributes(subjectClass,
               JavaTckSupport.attributeSet(subject,
                  subject.getAttributeHandle(subjectClass, attributeName)));
         } catch (Exception ignored) { }
         if (momSubscribed && monitorJoined && momClass != null && momAttributes != null) try {
            monitor.unsubscribeObjectClassAttributes(momClass, momAttributes);
         } catch (Exception ignored) { }
         if (applicationPublished && publisherJoined && publisherClass != null
               && publisherAttributes != null) try {
            publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         } catch (Exception ignored) { }
         if (subjectJoined) resign(subject);
         if (monitorJoined) resign(monitor);
         if (publisherJoined) resign(publisher);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (monitorConnected) disconnect(monitor);
         if (subjectConnected) disconnect(subject);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void sendAndCheckCount(RTIambassador publisher, RTIambassador subject,
         Recorder subjectRecorder, RTIambassador monitor, Recorder monitorRecorder,
         CallbackModel callbackModel, ObjectInstanceHandle object,
         AttributeHandle publisherAttribute, AttributeHandle subjectAttribute,
         FederateHandle publisherFederate, ObjectInstanceHandle subjectMomObject,
         AttributeHandle countAttribute, TransportationTypeHandle reliable,
         EncoderFactory encoder, int expectedCount, byte payload, String description)
         throws Exception {
      sendUpdate(publisher, subject, subjectRecorder, callbackModel, object, publisherAttribute,
         subjectAttribute, publisherFederate, payload, reliable, encoder, description);
      JavaTckSupport.check(requestCount(monitor, monitorRecorder, callbackModel, subjectMomObject,
            countAttribute, reliable, encoder, description + " counter request") == expectedCount,
         description + " did not increment HLAreflectionsReceived exactly once");
   }

   private static void sendUpdate(RTIambassador publisher, RTIambassador subject,
         Recorder recorder, CallbackModel model, ObjectInstanceHandle object,
         AttributeHandle publisherAttribute, AttributeHandle subjectAttribute,
         FederateHandle publisherFederate, byte payload, TransportationTypeHandle reliable,
         EncoderFactory encoder, String description) throws Exception {
      int first = recorder.reflections.size();
      byte[] encoded = encoder.createHLAbyte(payload).toByteArray();
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(publisherAttribute, encoded);
      publisher.updateAttributeValues(object, values, new byte[0]);
      await(subject, model,
         () -> findApplicationReflection(recorder, first, object, subjectAttribute, encoded) != null,
         description);
      Reflection reflection = findApplicationReflection(recorder, first, object, subjectAttribute,
         encoded);
      JavaTckSupport.check(reflection != null
            && countApplicationReflections(recorder, first, object, subjectAttribute, encoded) == 1
            && reflection.values.size() == 1 && reflection.tag.length == 0
            && reliable.equals(reflection.transportation)
            && publisherFederate.equals(reflection.producer)
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " returned missing, duplicate, or non-standard reflection data");
   }

   private static Reflection findApplicationReflection(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] expected) {
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         byte[] value = reflection.values.get(attribute);
         if (object.equals(reflection.object) && value != null && Arrays.equals(expected, value))
            return reflection;
      }
      return null;
   }

   private static int countApplicationReflections(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] expected) {
      int count = 0;
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         byte[] value = reflection.values.get(attribute);
         if (object.equals(reflection.object) && value != null && Arrays.equals(expected, value))
            ++count;
      }
      return count;
   }

   private static int requestCount(RTIambassador rti, Recorder recorder, CallbackModel model,
         ObjectInstanceHandle object, AttributeHandle attribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      int first = recorder.reflections.size();
      rti.requestAttributeValueUpdate(object, JavaTckSupport.attributeSet(rti, attribute),
         new byte[0]);
      await(rti, model, () -> findCount(recorder, first, object, attribute) != null, description);
      Reflection reflection = findCount(recorder, first, object, attribute);
      JavaTckSupport.check(reflection != null, description + " produced no MOM value");
      return verifyCount(reflection, attribute, reliable, encoder, description);
   }

   private static int verifyCount(Reflection reflection, AttributeHandle attribute,
         TransportationTypeHandle reliable, EncoderFactory encoder, String description)
         throws Exception {
      JavaTckSupport.check(reflection.values.size() == 1
            && reflection.values.containsKey(attribute) && reflection.tag.length == 0
            && reliable.equals(reflection.transportation) && reflection.producer == null
            && (reflection.regions == null || reflection.regions.isEmpty()),
         description + " returned non-standard MOM reflection metadata");
      HLAinteger32BE value = encoder.createHLAinteger32BE();
      value.decode(reflection.values.get(attribute));
      JavaTckSupport.check(value.getValue() >= 0, description + " returned a negative count");
      return value.getValue();
   }

   private static Reflection findCount(Recorder recorder, int first, ObjectInstanceHandle object,
         AttributeHandle attribute) {
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (object.equals(reflection.object) && reflection.values.containsKey(attribute))
            return reflection;
      }
      return null;
   }

   private static Reflection findCountAtLeast(Recorder recorder, int first,
         ObjectInstanceHandle object, AttributeHandle attribute, EncoderFactory encoder,
         int minimum) {
      for (int i = Math.min(first, recorder.reflections.size());
            i < recorder.reflections.size(); ++i) {
         Reflection reflection = recorder.reflections.get(i);
         if (!object.equals(reflection.object) || !reflection.values.containsKey(attribute))
            continue;
         HLAinteger32BE value = encoder.createHLAinteger32BE();
         try { value.decode(reflection.values.get(attribute)); }
         catch (Exception error) { throw new IllegalStateException("invalid MOM counter encoding", error); }
         if (value.getValue() >= minimum) return reflection;
      }
      return null;
   }

   private static void sendTiming(RTIambassador rti, InteractionClassHandle interaction,
         ParameterHandle federateParameter, ParameterHandle periodParameter, FederateHandle target,
         int period, EncoderFactory encoder) throws Exception {
      ParameterHandleValueMap values = rti.getParameterHandleValueMapFactory().create(2);
      values.put(federateParameter, encode(target));
      values.put(periodParameter, encoder.createHLAinteger32BE(period).toByteArray());
      rti.sendInteraction(interaction, values, new byte[0]);
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static void awaitDiscoveries(RTIambassador receiver, Recorder recorder,
         CallbackModel model, ObjectInstanceHandle... expected) throws Exception {
      await(receiver, model, () -> {
         for (ObjectInstanceHandle object : expected)
            if (!recorder.discoveries.contains(object)) return false;
         return true;
      }, "subject discovery of application objects");
   }

   private static void await(RTIambassador rti, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) rti.evokeCallback(0.0);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void resign(RTIambassador rti, ResignAction action) {
      try { rti.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
      }
   }

   private static final class TimedReflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;
      private TimedReflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions, LogicalTime<?, ?> time, OrderType sentOrder,
            OrderType receivedOrder, MessageRetractionHandle retraction) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
         this.retraction = retraction;
      }
   }

   private static final class Recorder {
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<TimedReflection> timedReflections = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private volatile LogicalTime<?, ?> timeRegulationEnabled;
      private volatile LogicalTime<?, ?> timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;

      private ObjectInstanceHandle momObject(AttributeHandle attribute, byte[] expected) {
         for (Reflection reflection : reflections) {
            byte[] value = reflection.values.get(attribute);
            if (value != null && Arrays.equals(expected, value)) return reflection.object;
         }
         return null;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timedReflections.add(new TimedReflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5], (LogicalTime<?, ?>) arguments[6],
                     (OrderType) arguments[7], (OrderType) arguments[8],
                     (MessageRetractionHandle) arguments[9]));
               } else if ("timeRegulationEnabled".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("timeConstrainedEnabled".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeConstrainedEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-instances-reflected MOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name))
                     return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
