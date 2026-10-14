package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies directed-interaction MOM counts exclude ordinary interaction deliveries. */
final class FederateMomDirectedInteractionsReceivedCountsTck {
   private static final String REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsReceived";
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsReceived";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomDirectedInteractionsReceivedCountsTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String interactionName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      Recorder senderRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-federate-mom-directed-interactions-received-counts-"
         + UUID.randomUUID();
      boolean receiverConnected = false, senderConnected = false, requesterConnected = false;
      boolean created = false, receiverJoined = false, senderJoined = false, requesterJoined = false;
      boolean receiverPublishedAttributes = false, senderSubscribedAttributes = false;
      boolean senderPublishedInteraction = false, receiverSubscribedInteraction = false;
      boolean senderPublishedDirected = false, receiverSubscribedDirected = false;
      boolean reportSubscribed = false;
      ObjectClassHandle receiverClass = null, senderClass = null;
      AttributeHandle receiverAttribute = null, senderAttribute = null;
      InteractionClassHandle receiverInteraction = null, senderInteraction = null;
      InteractionClassHandle reportClass = null;
      ObjectInstanceHandle target = null;
      try {
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         sender.connect(senderRecorder.proxy(), callbackModel);
         senderConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         receiver.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle receiverFederate = receiver.joinFederationExecution(
            "mom-directed-received-target", "java-tck", federation);
         receiverJoined = true;
         FederateHandle senderFederate = sender.joinFederationExecution(
            "mom-directed-received-sender", "java-tck", federation);
         senderJoined = true;
         FederateHandle requesterFederate = requester.joinFederationExecution(
            "mom-directed-received-requester", "java-tck", federation);
         requesterJoined = true;

         receiverClass = receiver.getObjectClassHandle(objectClassName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         senderClass = sender.getObjectClassHandle(objectClassName);
         senderAttribute = sender.getAttributeHandle(senderClass, attributeName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(receiverInteraction,
            parameterName);
         senderInteraction = sender.getInteractionClassHandle(interactionName);
         ParameterHandle senderParameter = sender.getParameterHandle(senderInteraction,
            parameterName);
         TransportationTypeHandle reliable = sender.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort = sender.getTransportationTypeHandle("HLAbestEffort");
         JavaTckSupport.check(objectClassName.equals(receiver.getObjectClassName(receiverClass))
               && attributeName.equals(receiver.getAttributeName(receiverClass, receiverAttribute))
               && interactionName.equals(receiver.getInteractionClassName(receiverInteraction))
               && parameterName.equals(receiver.getParameterName(receiverInteraction,
                  receiverParameter))
               && receiverClass.equals(senderClass)
               && receiverAttribute.equals(senderAttribute)
               && receiverInteraction.equals(senderInteraction)
               && receiverParameter.equals(senderParameter)
               && "HLAreliable".equals(sender.getTransportationTypeName(reliable))
               && "HLAbestEffort".equals(sender.getTransportationTypeName(bestEffort)),
            "adapter application handles or standard transports did not round-trip consistently");

         AttributeHandleSet receiverAttributes = JavaTckSupport.attributeSet(receiver,
            receiverAttribute);
         InteractionClassHandleSet interactionSet = JavaTckSupport.interactionSet(receiver,
            receiverInteraction);
         receiver.publishObjectClassAttributes(receiverClass, receiverAttributes);
         receiverPublishedAttributes = true;
         sender.subscribeObjectClassAttributes(senderClass,
            JavaTckSupport.attributeSet(sender, senderAttribute), "");
         senderSubscribedAttributes = true;
         sender.publishInteractionClass(senderInteraction);
         senderPublishedInteraction = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         receiverSubscribedInteraction = true;
         sender.publishObjectClassDirectedInteractions(senderClass,
            JavaTckSupport.interactionSet(sender, senderInteraction));
         senderPublishedDirected = true;
         receiver.subscribeObjectClassDirectedInteractions(receiverClass, interactionSet);
         receiverSubscribedDirected = true;

         target = receiver.registerObjectInstance(receiverClass);
         JavaTckSupport.check(target != null && target.encodedLength() > 0,
            "directed-interaction target registration returned an invalid handle");
         ObjectInstanceHandle expectedTarget = target;
         await(sender, callbackModel, () -> senderRecorder.discoveries.contains(expectedTarget),
            "sender discovery of the directed-interaction target");

         byte[] applicationPayload = {(byte) 0x61};
         ParameterHandleValueMap parameters = sender.getParameterHandleValueMapFactory().create(1);
         parameters.put(senderParameter, applicationPayload);
         sender.sendInteraction(senderInteraction, parameters, new byte[0]);
         await(receiver, callbackModel, () -> receiverRecorder.ordinary.size() >= 1,
            "ordinary interaction delivery");
         JavaTckSupport.check(receiverRecorder.ordinary.size() == 1,
            "receiver observed an unexpected number of ordinary interaction callbacks");
         ApplicationReceive ordinary = receiverRecorder.ordinary.get(0);
         verifyDelivery(ordinary, receiverInteraction, null, receiverParameter, applicationPayload,
            senderFederate, reliable, true);

         sender.sendDirectedInteraction(senderInteraction, target, parameters, new byte[0]);
         await(receiver, callbackModel, () -> receiverRecorder.directed.size() >= 1,
            "directed interaction delivery");
         JavaTckSupport.check(receiverRecorder.directed.size() == 1,
            "receiver observed an unexpected number of directed interaction callbacks");
         ApplicationReceive directed = receiverRecorder.directed.get(0);
         verifyDelivery(directed, receiverInteraction, target, receiverParameter,
            applicationPayload, senderFederate, reliable, true);

         int confirmationsBefore = senderRecorder.transportConfirmations.size();
         sender.requestInteractionTransportationTypeChange(senderInteraction, bestEffort);
         await(sender, callbackModel,
            () -> senderRecorder.transportConfirmations.size() > confirmationsBefore,
            "best-effort interaction transportation confirmation");
         List<TransportConfirmation> confirmations = senderRecorder.transportConfirmations;
         TransportConfirmation confirmation = confirmations.get(confirmations.size() - 1);
         JavaTckSupport.check(senderInteraction.equals(confirmation.interaction)
               && bestEffort.equals(confirmation.transportation),
            "directed-interaction transportation change did not confirm best effort");

         reportClass = requester.getInteractionClassHandle(REPORT_CLASS);
         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST_CLASS);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         ParameterHandle reportTransportation = requester.getParameterHandle(reportClass,
            "HLAtransportation");
         ParameterHandle reportCounts = requester.getParameterHandle(reportClass,
            "HLAinteractionCounts");
         TransportationTypeHandle reportReliable =
            requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(REQUEST_CLASS.equals(requester.getInteractionClassName(requestClass))
               && REPORT_CLASS.equals(requester.getInteractionClassName(reportClass))
               && "HLAfederate".equals(requester.getParameterName(requestClass, requestFederate))
               && "HLAtransportation".equals(requester.getParameterName(reportClass,
                  reportTransportation))
               && "HLAinteractionCounts".equals(requester.getParameterName(reportClass,
                  reportCounts))
               && "HLAreliable".equals(requester.getTransportationTypeName(reportReliable)),
            "standard directed-interaction MOM request/report handles did not round-trip");
         requester.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         EncoderFactory encoder = factory.getEncoderFactory();
         List<MomInteraction> receiverReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, reportClass, reportCounts,
            encode(receiverFederate), "receiver directed-interaction MOM reports");
         verifyReports(receiverReports, reportClass, reportTransportation, reportCounts,
            reportReliable, reliable, bestEffort, receiverInteraction, encoder, 1,
            "receiver");
         List<MomInteraction> requesterReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, reportClass, reportCounts,
            encode(requesterFederate), "requester empty directed-interaction MOM reports");
         verifyReports(requesterReports, reportClass, reportTransportation, reportCounts,
            reportReliable, reliable, bestEffort, receiverInteraction, encoder, 0,
            "empty requester");

         requester.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         receiver.unsubscribeObjectClassDirectedInteractions(receiverClass, interactionSet);
         receiverSubscribedDirected = false;
         receiver.unsubscribeInteractionClass(receiverInteraction);
         receiverSubscribedInteraction = false;
         sender.unsubscribeObjectClassAttributes(senderClass,
            JavaTckSupport.attributeSet(sender, senderAttribute));
         senderSubscribedAttributes = false;
         sender.unpublishObjectClassDirectedInteractions(senderClass,
            JavaTckSupport.interactionSet(sender, senderInteraction));
         senderPublishedDirected = false;
         sender.unpublishInteractionClass(senderInteraction);
         senderPublishedInteraction = false;
         receiver.unpublishObjectClassAttributes(receiverClass, receiverAttributes);
         receiverPublishedAttributes = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         sender.resignFederationExecution(ResignAction.NO_ACTION);
         senderJoined = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         receiver.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && requesterJoined && reportClass != null) try {
            requester.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (receiverSubscribedDirected && receiverJoined && receiverClass != null
               && receiverInteraction != null) try {
            receiver.unsubscribeObjectClassDirectedInteractions(receiverClass,
               JavaTckSupport.interactionSet(receiver, receiverInteraction));
         } catch (Exception ignored) { }
         if (receiverSubscribedInteraction && receiverJoined && receiverInteraction != null) try {
            receiver.unsubscribeInteractionClass(receiverInteraction);
         } catch (Exception ignored) { }
         if (senderSubscribedAttributes && senderJoined && senderClass != null
               && senderAttribute != null) try {
            sender.unsubscribeObjectClassAttributes(senderClass,
               JavaTckSupport.attributeSet(sender, senderAttribute));
         } catch (Exception ignored) { }
         if (senderPublishedDirected && senderJoined && senderClass != null
               && senderInteraction != null) try {
            sender.unpublishObjectClassDirectedInteractions(senderClass,
               JavaTckSupport.interactionSet(sender, senderInteraction));
         } catch (Exception ignored) { }
         if (senderPublishedInteraction && senderJoined && senderInteraction != null) try {
            sender.unpublishInteractionClass(senderInteraction);
         } catch (Exception ignored) { }
         if (receiverPublishedAttributes && receiverJoined && receiverClass != null
               && receiverAttribute != null) try {
            receiver.unpublishObjectClassAttributes(receiverClass,
               JavaTckSupport.attributeSet(receiver, receiverAttribute));
         } catch (Exception ignored) { }
         if (requesterJoined) resign(requester);
         if (senderJoined) resign(sender);
         if (receiverJoined) resign(receiver);
         if (created) try { receiver.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) disconnect(requester);
         if (senderConnected) disconnect(sender);
         if (receiverConnected) disconnect(receiver);
      }
   }

   /** Verifies sent-count reports exclude ordinary sends and include only directed sends. */
   static void runSentCounts(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador idle = factory.getRtiAmbassador();
      Recorder senderRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      Recorder idleRecorder = new Recorder();
      String federation = "java-tck-federate-mom-directed-interactions-sent-counts-"
         + UUID.randomUUID();
      boolean senderConnected = false, requesterConnected = false, idleConnected = false;
      boolean created = false, senderJoined = false, requesterJoined = false, idleJoined = false;
      boolean senderSubscribedAttributes = false, requesterPublishedAttributes = false;
      boolean senderPublishedInteraction = false, senderPublishedDirected = false;
      boolean reportSubscribed = false;
      ObjectClassHandle senderObjectClass = null, requesterObjectClass = null;
      AttributeHandle senderAttribute = null, requesterAttribute = null;
      InteractionClassHandle senderInteraction = null, reportClass = null;
      try {
         sender.connect(senderRecorder.proxy(), callbackModel);
         senderConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         idle.connect(idleRecorder.proxy(), callbackModel);
         idleConnected = true;
         sender.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle senderFederate = sender.joinFederationExecution(
            "mom-directed-sent-counts-sender", "java-tck", federation);
         senderJoined = true;
         FederateHandle requesterFederate = requester.joinFederationExecution(
            "mom-directed-sent-counts-requester", "java-tck", federation);
         requesterJoined = true;
         FederateHandle idleFederate = idle.joinFederationExecution(
            "mom-directed-sent-counts-idle", "java-tck", federation);
         idleJoined = true;

         senderObjectClass = sender.getObjectClassHandle(objectClassName);
         senderAttribute = sender.getAttributeHandle(senderObjectClass, attributeName);
         requesterObjectClass = requester.getObjectClassHandle(objectClassName);
         requesterAttribute = requester.getAttributeHandle(requesterObjectClass, attributeName);
         senderInteraction = sender.getInteractionClassHandle(interactionName);
         ParameterHandle senderParameter = sender.getParameterHandle(senderInteraction,
            parameterName);
         JavaTckSupport.check(senderObjectClass.equals(requesterObjectClass)
               && senderAttribute.equals(requesterAttribute)
               && objectClassName.equals(sender.getObjectClassName(senderObjectClass))
               && objectClassName.equals(requester.getObjectClassName(requesterObjectClass))
               && attributeName.equals(sender.getAttributeName(senderObjectClass, senderAttribute))
               && attributeName.equals(requester.getAttributeName(requesterObjectClass,
                  requesterAttribute))
               && interactionName.equals(sender.getInteractionClassName(senderInteraction))
               && parameterName.equals(sender.getParameterName(senderInteraction, senderParameter)),
            "adapter application handles for directed sent counts did not round-trip");
         TransportationTypeHandle reliable = sender.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort = sender.getTransportationTypeHandle("HLAbestEffort");
         JavaTckSupport.check("HLAreliable".equals(sender.getTransportationTypeName(reliable))
               && "HLAbestEffort".equals(sender.getTransportationTypeName(bestEffort)),
            "standard interaction transportation handles did not round-trip");

         requester.publishObjectClassAttributes(requesterObjectClass,
            JavaTckSupport.attributeSet(requester, requesterAttribute));
         requesterPublishedAttributes = true;
         sender.subscribeObjectClassAttributes(senderObjectClass,
            JavaTckSupport.attributeSet(sender, senderAttribute), "");
         senderSubscribedAttributes = true;
         sender.publishInteractionClass(senderInteraction);
         senderPublishedInteraction = true;
         sender.publishObjectClassDirectedInteractions(senderObjectClass,
            JavaTckSupport.interactionSet(sender, senderInteraction));
         senderPublishedDirected = true;
         ObjectInstanceHandle target = requester.registerObjectInstance(requesterObjectClass);
         await(sender, callbackModel, () -> senderRecorder.discoveries.contains(target),
            "sender discovery of the directed-interaction target");

         sendApplication(sender, senderInteraction, senderParameter, (byte) 0x81);
         sendDirected(sender, senderInteraction, target, senderParameter, (byte) 0x82);
         sendDirected(sender, senderInteraction, target, senderParameter, (byte) 0x83);
         int confirmationsBefore = senderRecorder.transportConfirmations.size();
         sender.requestInteractionTransportationTypeChange(senderInteraction, bestEffort);
         await(sender, callbackModel,
            () -> senderRecorder.transportConfirmations.size() > confirmationsBefore,
            "best-effort interaction transportation confirmation");
         TransportConfirmation confirmation = senderRecorder.transportConfirmations.get(
            senderRecorder.transportConfirmations.size() - 1);
         JavaTckSupport.check(senderInteraction.equals(confirmation.interaction)
               && bestEffort.equals(confirmation.transportation),
            "directed sent-count transportation change did not confirm best effort");
         sendApplication(sender, senderInteraction, senderParameter, (byte) 0x84);

         reportClass = requester.getInteractionClassHandle(REPORT_CLASS);
         InteractionClassHandle requestClass = requester.getInteractionClassHandle(
            "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
               + "HLArequestDirectedInteractionsSent");
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         ParameterHandle reportTransportation = requester.getParameterHandle(reportClass,
            "HLAtransportation");
         ParameterHandle reportCounts = requester.getParameterHandle(reportClass,
            "HLAinteractionCounts");
         TransportationTypeHandle reportReliable =
            requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(REPORT_CLASS.equals(requester.getInteractionClassName(reportClass))
               && "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
                  .concat("HLArequestDirectedInteractionsSent")
                  .equals(requester.getInteractionClassName(requestClass))
               && "HLAfederate".equals(requester.getParameterName(requestClass, requestFederate))
               && "HLAtransportation".equals(requester.getParameterName(reportClass,
                  reportTransportation))
               && "HLAinteractionCounts".equals(requester.getParameterName(reportClass,
                  reportCounts))
               && "HLAreliable".equals(requester.getTransportationTypeName(reportReliable)),
            "standard directed sent-count MOM handles did not round-trip");
         requester.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         EncoderFactory encoder = factory.getEncoderFactory();
         List<MomInteraction> senderReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, reportClass, reportCounts,
            encode(senderFederate), "sender directed-interaction sent-count MOM reports");
         verifyReports(senderReports, reportClass, reportTransportation, reportCounts,
            reportReliable, reliable, bestEffort, senderInteraction, encoder, 2, "sender");
         List<MomInteraction> idleReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, reportClass, reportCounts,
            encode(idleFederate), "idle directed-interaction sent-count MOM reports");
         verifyReports(idleReports, reportClass, reportTransportation, reportCounts,
            reportReliable, reliable, bestEffort, senderInteraction, encoder, 0, "empty idle");

         requester.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         sender.unsubscribeObjectClassAttributes(senderObjectClass,
            JavaTckSupport.attributeSet(sender, senderAttribute));
         senderSubscribedAttributes = false;
         requester.unpublishObjectClassAttributes(requesterObjectClass,
            JavaTckSupport.attributeSet(requester, requesterAttribute));
         requesterPublishedAttributes = false;
         sender.unpublishObjectClassDirectedInteractions(senderObjectClass,
            JavaTckSupport.interactionSet(sender, senderInteraction));
         senderPublishedDirected = false;
         sender.unpublishInteractionClass(senderInteraction);
         senderPublishedInteraction = false;
         requester.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         requesterJoined = false;
         idle.resignFederationExecution(ResignAction.NO_ACTION);
         idleJoined = false;
         sender.resignFederationExecution(ResignAction.NO_ACTION);
         senderJoined = false;
         sender.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && requesterJoined && reportClass != null) try {
            requester.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (senderSubscribedAttributes && senderJoined && senderObjectClass != null
               && senderAttribute != null) try {
            sender.unsubscribeObjectClassAttributes(senderObjectClass,
               JavaTckSupport.attributeSet(sender, senderAttribute));
         } catch (Exception ignored) { }
         if (requesterPublishedAttributes && requesterJoined && requesterObjectClass != null
               && requesterAttribute != null) try {
            requester.unpublishObjectClassAttributes(requesterObjectClass,
               JavaTckSupport.attributeSet(requester, requesterAttribute));
         } catch (Exception ignored) { }
         if (senderPublishedDirected && senderJoined && senderObjectClass != null
               && senderInteraction != null) try {
            sender.unpublishObjectClassDirectedInteractions(senderObjectClass,
               JavaTckSupport.interactionSet(sender, senderInteraction));
         } catch (Exception ignored) { }
         if (senderPublishedInteraction && senderJoined && senderInteraction != null) try {
            sender.unpublishInteractionClass(senderInteraction);
         } catch (Exception ignored) { }
         if (requesterJoined) resign(requester);
         if (idleJoined) resign(idle);
         if (senderJoined) resign(sender);
         if (created) try { sender.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (idleConnected) disconnect(idle);
         if (requesterConnected) disconnect(requester);
         if (senderConnected) disconnect(sender);
      }
   }

   private static void sendApplication(RTIambassador sender, InteractionClassHandle interaction,
         ParameterHandle parameter, byte value) throws Exception {
      ParameterHandleValueMap values = sender.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, new byte[] {value});
      sender.sendInteraction(interaction, values, new byte[0]);
   }

   private static void sendDirected(RTIambassador sender, InteractionClassHandle interaction,
         ObjectInstanceHandle target, ParameterHandle parameter, byte value) throws Exception {
      ParameterHandleValueMap values = sender.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, new byte[] {value});
      sender.sendDirectedInteraction(interaction, target, values, new byte[0]);
   }

   private static void verifyDelivery(ApplicationReceive receive,
         InteractionClassHandle expectedInteraction, ObjectInstanceHandle expectedObject,
         ParameterHandle expectedParameter, byte[] expectedPayload, FederateHandle expectedProducer,
         TransportationTypeHandle expectedTransportation, boolean checkObject) {
      JavaTckSupport.check(expectedInteraction.equals(receive.interaction)
            && (!checkObject || (expectedObject == null ? receive.object == null
               : expectedObject.equals(receive.object)))
            && receive.values.size() == 1 && receive.values.containsKey(expectedParameter)
            && Arrays.equals(expectedPayload, receive.values.get(expectedParameter))
            && receive.tag.length == 0 && expectedTransportation.equals(receive.transportation)
            && expectedProducer.equals(receive.producer)
            && (receive.regions == null || receive.regions.isEmpty()),
         "directed-interaction callback returned non-standard target, payload, or metadata");
   }

   private static List<MomInteraction> requestReports(RTIambassador requester, Recorder recorder,
         CallbackModel callbackModel, InteractionClassHandle requestClass,
         ParameterHandle requestFederate, InteractionClassHandle reportClass,
         ParameterHandle reportCounts, byte[] federate, String description) throws Exception {
      recorder.momInteractions.clear();
      ParameterHandleValueMap request = requester.getParameterHandleValueMapFactory().create(1);
      request.put(requestFederate, federate);
      requester.sendInteraction(requestClass, request, new byte[0]);
      await(requester, callbackModel,
         () -> recorder.momInteractions.stream().filter(i -> reportClass.equals(i.interaction)
            && i.parameters.containsKey(reportCounts)).count() >= 2,
         description);
      List<MomInteraction> reports = new ArrayList<>();
      for (MomInteraction interaction : recorder.momInteractions)
         if (reportClass.equals(interaction.interaction)) reports.add(interaction);
      return reports;
   }

   private static void verifyReports(List<MomInteraction> reports,
         InteractionClassHandle reportClass, ParameterHandle reportTransportation,
         ParameterHandle reportCounts, TransportationTypeHandle reportReliable,
         TransportationTypeHandle reliable, TransportationTypeHandle bestEffort,
         InteractionClassHandle expectedInteraction, EncoderFactory encoder,
         int expectedReliableCount, String description) throws Exception {
      JavaTckSupport.check(reports.size() == 2,
         description + " did not return one report for each standard transportation type");
      boolean sawReliable = false, sawBestEffort = false;
      for (MomInteraction report : reports) {
         JavaTckSupport.check(reportClass.equals(report.interaction) && report.parameters.size() == 2
               && report.parameters.containsKey(reportTransportation)
               && report.parameters.containsKey(reportCounts) && report.tag.length == 0
               && report.producer == null && report.regionsEmpty
               && reportReliable.equals(report.transportation),
            description + " returned non-standard MOM report metadata");
         byte[] transport = report.parameters.get(reportTransportation);
         if (Arrays.equals(transport, encode(reliable))) {
            JavaTckSupport.check(!sawReliable, description + " returned duplicate reliable buckets");
            sawReliable = true;
            verifyCountArray(encoder, report.parameters.get(reportCounts), expectedInteraction,
               expectedReliableCount, description + " reliable count");
         } else if (Arrays.equals(transport, encode(bestEffort))) {
            JavaTckSupport.check(!sawBestEffort,
               description + " returned duplicate best-effort buckets");
            sawBestEffort = true;
            verifyCountArray(encoder, report.parameters.get(reportCounts), expectedInteraction,
               0, description + " best-effort count");
         } else throw new AssertionError(description + " returned an unknown transportation bucket");
      }
      JavaTckSupport.check(sawReliable && sawBestEffort,
         description + " omitted one of the standard transportation buckets");
   }

   private static void verifyCountArray(EncoderFactory encoder, byte[] encoded,
         InteractionClassHandle expectedInteraction, int expectedCount, String description)
         throws Exception {
      HLAfixedRecord recordPrototype = encoder.createHLAfixedRecord();
      recordPrototype.add(encoder.createHLAvariableArray(index -> encoder.createHLAbyte(),
         encoder.createHLAbyte()));
      recordPrototype.add(encoder.createHLAinteger32BE());
      HLAvariableArray<HLAfixedRecord> counts = encoder.createHLAvariableArray(index -> {
         HLAfixedRecord record = encoder.createHLAfixedRecord();
         record.add(encoder.createHLAvariableArray(inner -> encoder.createHLAbyte(),
            encoder.createHLAbyte()));
         record.add(encoder.createHLAinteger32BE());
         return record;
      }, recordPrototype);
      if (encoded.length != 0) counts.decode(encoded);
      if (expectedCount == 0) {
         JavaTckSupport.check(counts.size() == 0,
            description + " encoded a non-empty list for a NULL report");
         return;
      }
      JavaTckSupport.check(counts.size() == 1,
         description + " returned the wrong number of directed interaction count rows");
      HLAfixedRecord record = counts.get(0);
      @SuppressWarnings("unchecked")
      HLAvariableArray<HLAbyte> classBytes = (HLAvariableArray<HLAbyte>) record.get(0);
      JavaTckSupport.check(Arrays.equals(encode(expectedInteraction), classBytes.toByteArray())
            && ((HLAinteger32BE) record.get(1)).getValue() == expectedCount,
         description + " returned the wrong interaction class or directed count");
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }
   private static byte[] encode(InteractionClassHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }
   private static byte[] encode(TransportationTypeHandle handle) throws Exception {
      byte[] bytes = new byte[handle.encodedLength()];
      handle.encode(bytes, 0);
      return bytes;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }
   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class ApplicationReceive {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle object;
      private final ParameterHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private ApplicationReceive(InteractionClassHandle interaction, ObjectInstanceHandle object,
            ParameterHandleValueMap values, byte[] tag, TransportationTypeHandle transportation,
            FederateHandle producer, RegionHandleSet regions) {
         this.interaction = interaction;
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
      }
   }
   private static final class MomInteraction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private MomInteraction(InteractionClassHandle interaction, ParameterHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.interaction = interaction;
         for (ParameterHandle parameter : values.keySet())
            parameters.put(parameter, values.get(parameter).clone());
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regions == null || regions.isEmpty();
      }
   }
   private static final class TransportConfirmation {
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;
      private TransportConfirmation(InteractionClassHandle interaction,
            TransportationTypeHandle transportation) {
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }
   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ApplicationReceive> ordinary = new CopyOnWriteArrayList<>();
      private final List<ApplicationReceive> directed = new CopyOnWriteArrayList<>();
      private final List<MomInteraction> momInteractions = new CopyOnWriteArrayList<>();
      private final List<TransportConfirmation> transportConfirmations =
         new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  ApplicationReceive event = new ApplicationReceive(
                     (InteractionClassHandle) arguments[0], null,
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]);
                  ordinary.add(event);
                  momInteractions.add(new MomInteraction(event.interaction, event.values,
                     event.tag, event.transportation, event.producer, event.regions));
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  directed.add(new ApplicationReceive((InteractionClassHandle) arguments[0],
                     (ObjectInstanceHandle) arguments[1], (ParameterHandleValueMap) arguments[2],
                     (byte[]) arguments[3], (TransportationTypeHandle) arguments[4],
                     (FederateHandle) arguments[5], null));
               } else if ("confirmInteractionTransportationTypeChange".equals(name)
                     && arguments != null && arguments.length == 2) {
                  transportConfirmations.add(new TransportConfirmation(
                     (InteractionClassHandle) arguments[0],
                     (TransportationTypeHandle) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK directed-interaction count callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
