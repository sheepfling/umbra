package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
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

/** Verifies received-interaction MOM counts across standard transportation types. */
final class FederateMomInteractionsReceivedCountsTck {
   private static final String REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived";
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsReceived";
   private static final String SENT_REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent";
   private static final String SENT_REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsSent";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomInteractionsReceivedCountsTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder receiverRecorder = new Recorder();
      Recorder senderRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-federate-mom-interactions-received-counts-"
         + UUID.randomUUID();
      boolean receiverConnected = false, senderConnected = false, requesterConnected = false;
      boolean created = false, receiverJoined = false, senderJoined = false, requesterJoined = false;
      boolean published = false, receiverSubscribed = false, reportSubscribed = false;
      InteractionClassHandle receiverInteraction = null, senderInteraction = null;
      InteractionClassHandle reportClass = null;
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
            "mom-interactions-received-target", "java-tck", federation);
         receiverJoined = true;
         FederateHandle senderFederate = sender.joinFederationExecution(
            "mom-interactions-received-sender", "java-tck", federation);
         senderJoined = true;
         FederateHandle requesterFederate = requester.joinFederationExecution(
            "mom-interactions-received-requester", "java-tck", federation);
         requesterJoined = true;

         receiverInteraction = receiver.getInteractionClassHandle(interactionName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         senderInteraction = sender.getInteractionClassHandle(interactionName);
         ParameterHandle senderParameter = sender.getParameterHandle(
            senderInteraction, parameterName);
         TransportationTypeHandle reliable = sender.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort = sender.getTransportationTypeHandle("HLAbestEffort");
         JavaTckSupport.check(interactionName.equals(receiver.getInteractionClassName(
               receiverInteraction))
               && parameterName.equals(receiver.getParameterName(receiverInteraction,
                  receiverParameter))
               && interactionName.equals(sender.getInteractionClassName(senderInteraction))
               && parameterName.equals(sender.getParameterName(senderInteraction,
                  senderParameter))
               && receiverInteraction.equals(senderInteraction)
               && receiverParameter.equals(senderParameter)
               && "HLAreliable".equals(sender.getTransportationTypeName(reliable))
               && "HLAbestEffort".equals(sender.getTransportationTypeName(bestEffort)),
            "adapter interaction handles or standard transportation handles failed round-trip");

         sender.publishInteractionClass(senderInteraction);
         published = true;
         receiver.subscribeInteractionClass(receiverInteraction);
         receiverSubscribed = true;
         send(sender, senderInteraction, senderParameter, factory.getEncoderFactory(), (byte) 0x61);
         await(receiver, callbackModel,
            () -> receiverRecorder.interactions.size() >= 1,
            "initial reliable interaction delivery");
         verifyApplicationInteractions(receiverRecorder.interactions, receiverInteraction,
            receiverParameter, senderFederate, reliable, bestEffort, 1, 0);

         int confirmationsBefore = senderRecorder.transportConfirmations.size();
         sender.requestInteractionTransportationTypeChange(senderInteraction, bestEffort);
         await(sender, callbackModel,
            () -> senderRecorder.transportConfirmations.size() > confirmationsBefore,
            "best-effort interaction transportation confirmation");
         List<TransportConfirmation> confirmations = senderRecorder.transportConfirmations;
         TransportConfirmation confirmation = confirmations.get(confirmations.size() - 1);
         JavaTckSupport.check(senderInteraction.equals(confirmation.interaction)
               && bestEffort.equals(confirmation.transportation),
            "interaction transportation change did not confirm best effort for the published class");

         send(sender, senderInteraction, senderParameter, factory.getEncoderFactory(), (byte) 0x62);
         send(sender, senderInteraction, senderParameter, factory.getEncoderFactory(), (byte) 0x63);
         await(receiver, callbackModel,
            () -> receiverRecorder.interactions.size() >= 3,
            "best-effort interaction deliveries");
         verifyApplicationInteractions(receiverRecorder.interactions, receiverInteraction,
            receiverParameter, senderFederate, reliable, bestEffort, 1, 2);

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
            "standard interactions-received MOM request/report handles did not round-trip");
         requester.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         EncoderFactory encoder = factory.getEncoderFactory();
         List<Interaction> receiverReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, reportClass, reportCounts,
            encode(receiverFederate), "receiver interactions-received MOM reports");
         verifyReports(receiverReports, reportClass, reportTransportation, reportCounts,
            reportReliable, reliable, bestEffort, receiverInteraction, encoder, true,
            "receiver");
         List<Interaction> requesterReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, reportClass, reportCounts,
            encode(requesterFederate), "requester empty interactions-received MOM reports");
         verifyReports(requesterReports, reportClass, reportTransportation, reportCounts,
            reportReliable, reliable, bestEffort, receiverInteraction, encoder, false,
            "empty requester");

         requester.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         receiver.unsubscribeInteractionClass(receiverInteraction);
         receiverSubscribed = false;
         sender.unpublishInteractionClass(senderInteraction);
         published = false;
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
         if (receiverSubscribed && receiverJoined && receiverInteraction != null) try {
            receiver.unsubscribeInteractionClass(receiverInteraction);
         } catch (Exception ignored) { }
         if (published && senderJoined && senderInteraction != null) try {
            sender.unpublishInteractionClass(senderInteraction);
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

   /** Verifies sent-count MOM reports across the confirmed standard transports. */
   static void runSent(RtiFactory factory, String fom, String mim, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador idle = factory.getRtiAmbassador();
      Recorder senderRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      Recorder idleRecorder = new Recorder();
      String federation = "java-tck-federate-mom-interactions-sent-counts-" + UUID.randomUUID();
      boolean senderConnected = false, requesterConnected = false, idleConnected = false;
      boolean created = false, senderJoined = false, requesterJoined = false, idleJoined = false;
      boolean published = false, reportSubscribed = false;
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
            "mom-interactions-sent-counts-sender", "java-tck", federation);
         senderJoined = true;
         FederateHandle requesterFederate = requester.joinFederationExecution(
            "mom-interactions-sent-counts-requester", "java-tck", federation);
         requesterJoined = true;
         FederateHandle idleFederate = idle.joinFederationExecution(
            "mom-interactions-sent-counts-idle", "java-tck", federation);
         idleJoined = true;

         senderInteraction = sender.getInteractionClassHandle(interactionName);
         ParameterHandle senderParameter = sender.getParameterHandle(senderInteraction,
            parameterName);
         TransportationTypeHandle reliable = sender.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort = sender.getTransportationTypeHandle("HLAbestEffort");
         JavaTckSupport.check(interactionName.equals(
               sender.getInteractionClassName(senderInteraction))
               && parameterName.equals(sender.getParameterName(senderInteraction, senderParameter))
               && "HLAreliable".equals(sender.getTransportationTypeName(reliable))
               && "HLAbestEffort".equals(sender.getTransportationTypeName(bestEffort)),
            "adapter interaction and standard transportation handles did not round-trip");
         sender.publishInteractionClass(senderInteraction);
         published = true;
         send(sender, senderInteraction, senderParameter, factory.getEncoderFactory(),
            (byte) 0x81);
         int confirmationsBefore = senderRecorder.transportConfirmations.size();
         sender.requestInteractionTransportationTypeChange(senderInteraction, bestEffort);
         await(sender, callbackModel,
            () -> senderRecorder.transportConfirmations.size() > confirmationsBefore,
            "best-effort sent-count transportation confirmation");
         TransportConfirmation confirmation = senderRecorder.transportConfirmations.get(
            senderRecorder.transportConfirmations.size() - 1);
         JavaTckSupport.check(senderInteraction.equals(confirmation.interaction)
               && bestEffort.equals(confirmation.transportation),
            "sent-count transportation change did not confirm best effort");
         send(sender, senderInteraction, senderParameter, factory.getEncoderFactory(),
            (byte) 0x82);
         send(sender, senderInteraction, senderParameter, factory.getEncoderFactory(),
            (byte) 0x83);

         reportClass = requester.getInteractionClassHandle(SENT_REPORT_CLASS);
         InteractionClassHandle requestClass =
            requester.getInteractionClassHandle(SENT_REQUEST_CLASS);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         ParameterHandle reportTransportation = requester.getParameterHandle(reportClass,
            "HLAtransportation");
         ParameterHandle reportCounts = requester.getParameterHandle(reportClass,
            "HLAinteractionCounts");
         TransportationTypeHandle reportReliable =
            requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(SENT_REQUEST_CLASS.equals(
               requester.getInteractionClassName(requestClass))
               && SENT_REPORT_CLASS.equals(requester.getInteractionClassName(reportClass))
               && "HLAfederate".equals(requester.getParameterName(requestClass, requestFederate))
               && "HLAtransportation".equals(requester.getParameterName(reportClass,
                  reportTransportation))
               && "HLAinteractionCounts".equals(requester.getParameterName(reportClass,
                  reportCounts))
               && "HLAreliable".equals(requester.getTransportationTypeName(reportReliable)),
            "standard interactions-sent MOM request/report handles did not round-trip");
         requester.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         EncoderFactory encoder = factory.getEncoderFactory();
         List<Interaction> senderReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, reportClass, reportCounts,
            encode(senderFederate), "sender interactions-sent MOM reports");
         verifyReports(senderReports, reportClass, reportTransportation, reportCounts,
            reportReliable, reliable, bestEffort, senderInteraction, encoder, true, "sender");
         List<Interaction> idleReports = requestReports(requester, requesterRecorder,
            callbackModel, requestClass, requestFederate, reportClass, reportCounts,
            encode(idleFederate), "idle interactions-sent MOM reports");
         verifyReports(idleReports, reportClass, reportTransportation, reportCounts,
            reportReliable, reliable, bestEffort, senderInteraction, encoder, false,
            "empty idle");

         requester.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         sender.unpublishInteractionClass(senderInteraction);
         published = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
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
         if (published && senderJoined && senderInteraction != null) try {
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

   private static void send(RTIambassador sender, InteractionClassHandle interaction,
         ParameterHandle parameter, EncoderFactory encoder, byte value) throws Exception {
      ParameterHandleValueMap values = sender.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, encoder.createHLAbyte(value).toByteArray());
      sender.sendInteraction(interaction, values, new byte[0]);
   }

   private static void verifyApplicationInteractions(List<ApplicationInteraction> interactions,
         InteractionClassHandle interaction, ParameterHandle parameter, FederateHandle producer,
         TransportationTypeHandle reliable, TransportationTypeHandle bestEffort,
         int reliableExpected, int bestEffortExpected) throws Exception {
      JavaTckSupport.check(interactions.size() == reliableExpected + bestEffortExpected,
         "receiver observed an unexpected number of application interactions");
      int reliableCount = 0, bestEffortCount = 0;
      for (ApplicationInteraction event : interactions) {
         JavaTckSupport.check(interaction.equals(event.interaction) && event.values.size() == 1
               && event.values.containsKey(parameter) && event.tag.length == 0
               && producer.equals(event.producer)
               && (event.regions == null || event.regions.isEmpty()),
            "received application interaction had non-standard class, parameter, or metadata");
         if (reliable.equals(event.transportation)) reliableCount++;
         else if (bestEffort.equals(event.transportation)) bestEffortCount++;
         else throw new AssertionError("received application interaction used an unknown transport");
         byte[] payload = event.values.get(parameter);
         JavaTckSupport.check(payload.length == 1
               && (payload[0] == 0x61 || payload[0] == 0x62 || payload[0] == 0x63),
            "received application interaction returned an unknown payload");
      }
      JavaTckSupport.check(reliableCount == reliableExpected && bestEffortCount == bestEffortExpected,
         "received interactions were not counted under their confirmed transportation types");
   }

   private static List<Interaction> requestReports(RTIambassador requester, Recorder recorder,
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
      List<Interaction> reports = new ArrayList<>();
      for (Interaction interaction : recorder.momInteractions)
         if (reportClass.equals(interaction.interaction)) reports.add(interaction);
      return reports;
   }

   private static void verifyReports(List<Interaction> reports,
         InteractionClassHandle reportClass, ParameterHandle reportTransportation,
         ParameterHandle reportCounts, TransportationTypeHandle reportReliable,
         TransportationTypeHandle reliable, TransportationTypeHandle bestEffort,
         InteractionClassHandle expectedInteraction, EncoderFactory encoder,
         boolean expectCounts, String description) throws Exception {
      JavaTckSupport.check(reports.size() == 2,
         description + " did not return exactly one report for each standard transport");
      boolean sawReliable = false, sawBestEffort = false;
      for (Interaction report : reports) {
         JavaTckSupport.check(reportClass.equals(report.interaction) && report.parameters.size() == 2
               && report.parameters.containsKey(reportTransportation)
               && report.parameters.containsKey(reportCounts)
               && report.tag.length == 0 && report.producer == null && report.regionsEmpty
               && reportReliable.equals(report.transportation),
            description + " returned non-standard MOM report metadata");
         byte[] transportationBytes = report.parameters.get(reportTransportation);
         if (Arrays.equals(transportationBytes, encode(reliable))) {
            JavaTckSupport.check(!sawReliable, description + " returned duplicate reliable reports");
            sawReliable = true;
            verifyCountArray(encoder, report.parameters.get(reportCounts), expectedInteraction,
               expectCounts ? 1 : 0, description + " reliable count");
         } else if (Arrays.equals(transportationBytes, encode(bestEffort))) {
            JavaTckSupport.check(!sawBestEffort,
               description + " returned duplicate best-effort reports");
            sawBestEffort = true;
            verifyCountArray(encoder, report.parameters.get(reportCounts), expectedInteraction,
               expectCounts ? 2 : 0, description + " best-effort count");
         } else throw new AssertionError(description + " reported an unknown transportation type");
      }
      JavaTckSupport.check(sawReliable && sawBestEffort,
         description + " omitted a standard transportation bucket");
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
            description + " encoded a non-empty list for a NULL response");
         return;
      }
      JavaTckSupport.check(counts.size() == 1,
         description + " returned the wrong number of positive interaction counts");
      HLAfixedRecord record = counts.get(0);
      @SuppressWarnings("unchecked")
      HLAvariableArray<HLAbyte> classBytes = (HLAvariableArray<HLAbyte>) record.get(0);
      JavaTckSupport.check(Arrays.equals(encode(expectedInteraction), classBytes.toByteArray())
            && ((HLAinteger32BE) record.get(1)).getValue() == expectedCount,
         description + " returned the wrong interaction class or count");
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

   private static final class ApplicationInteraction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;
      private ApplicationInteraction(InteractionClassHandle interaction,
            ParameterHandleValueMap values, byte[] tag, TransportationTypeHandle transportation,
            FederateHandle producer, RegionHandleSet regions) {
         this.interaction = interaction;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions == null ? null : (RegionHandleSet) regions.clone();
      }
   }
   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private Interaction(InteractionClassHandle interaction, ParameterHandleValueMap values,
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
      private final List<ApplicationInteraction> interactions = new CopyOnWriteArrayList<>();
      private final List<Interaction> momInteractions = new CopyOnWriteArrayList<>();
      private final List<TransportConfirmation> transportConfirmations =
         new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  ApplicationInteraction event = new ApplicationInteraction(
                     (InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]);
                  interactions.add(event);
                  momInteractions.add(new Interaction(event.interaction, event.values,
                     event.tag, event.transportation, event.producer, event.regions));
               } else if ("confirmInteractionTransportationTypeChange".equals(name)
                     && arguments != null && arguments.length == 2) {
                  transportConfirmations.add(new TransportConfirmation(
                     (InteractionClassHandle) arguments[0],
                     (TransportationTypeHandle) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK received-interaction count callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
