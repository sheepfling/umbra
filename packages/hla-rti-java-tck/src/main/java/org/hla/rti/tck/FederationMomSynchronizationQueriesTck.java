package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** P2 synchronization-point state queried through the standard federation MOM. */
final class FederationMomSynchronizationQueriesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String POINTS_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPoints";
   private static final String POINTS_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPoints";
   private static final String STATUS_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus";
   private static final String STATUS_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPointStatus";

   private FederationMomSynchronizationQueriesTck() {
   }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-mom-sync-queries-" + UUID.randomUUID();
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      boolean pointsSubscribed = false;
      boolean statusSubscribed = false;
      InteractionClassHandle pointsReportClass = null;
      InteractionClassHandle statusReportClass = null;
      try {
         check(mim != null && !mim.isEmpty(),
            "federation MOM synchronization queries require an adapter-supplied standard MIM");
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            "java-tck-sync-mom-subject", "tck-type", federation);
         subjectJoined = true;
         FederateHandle observerHandle = observer.joinFederationExecution(
            "java-tck-sync-mom-observer", "tck-type", federation);
         observerJoined = true;
         subject.setServiceReportingSwitch(false);
         observer.setServiceReportingSwitch(false);

         byte[] tag = new byte[] {0x4d, 0x4f, 0x4d, 0x53};
         byte[] emptyTag = new byte[0];
         String label = "java-tck-mom-synchronization-point";
         subject.registerFederationSynchronizationPoint(label, tag);
         await(subject, observer, callbackModel,
            () -> subjectRecorder.registrations.contains(label)
               && recorder.announcements.containsKey(label),
            "standard synchronization registration and announcement");
         check(Arrays.equals(tag, recorder.announcements.get(label)),
            "standard synchronization announcement changed the tag");

         InteractionClassHandle pointsRequestClass = subject.getInteractionClassHandle(POINTS_REQUEST);
         pointsReportClass = observer.getInteractionClassHandle(POINTS_REPORT);
         ParameterHandle pointsReportParameter = observer.getParameterHandle(
            pointsReportClass, "HLAsyncPoints");
         InteractionClassHandle statusRequestClass = subject.getInteractionClassHandle(STATUS_REQUEST);
         ParameterHandle statusRequestParameter = subject.getParameterHandle(
            statusRequestClass, "HLAsyncPointName");
         statusReportClass = observer.getInteractionClassHandle(STATUS_REPORT);
         ParameterHandle statusReportNameParameter = observer.getParameterHandle(
            statusReportClass, "HLAsyncPointName");
         ParameterHandle statusReportFederatesParameter = observer.getParameterHandle(
            statusReportClass, "HLAsyncPointFederates");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         check(pointsRequestClass != null && pointsReportClass != null
               && pointsReportParameter != null && statusRequestClass != null
               && statusRequestParameter != null && statusReportClass != null
               && statusReportNameParameter != null && statusReportFederatesParameter != null
               && reliable != null,
            "standard synchronization MOM lookup returned a null handle");
         observer.subscribeInteractionClass(pointsReportClass);
         pointsSubscribed = true;
         observer.subscribeInteractionClass(statusReportClass);
         statusSubscribed = true;

         check(requestPointList(subject, observer, recorder, callbackModel, encoder,
               pointsRequestClass, pointsReportClass, pointsReportParameter, reliable, emptyTag)
               .equals(java.util.Collections.singletonList(label)),
            "standard MOM synchronization-point list did not contain the pending point");
         Map<FederateHandle, Integer> pending = requestStatus(subject, observer, recorder,
            callbackModel, encoder, statusRequestClass, statusRequestParameter, statusReportClass,
            statusReportNameParameter, statusReportFederatesParameter, reliable, label, emptyTag);
         check(pending.size() == 2 && Integer.valueOf(2).equals(pending.get(subjectHandle))
               && Integer.valueOf(2).equals(pending.get(observerHandle)),
            "standard MOM synchronization status did not report both members pending");

         subject.synchronizationPointAchieved(label);
         pump(subject, observer, callbackModel, 8);
         Map<FederateHandle, Integer> partial = requestStatus(subject, observer, recorder,
            callbackModel, encoder, statusRequestClass, statusRequestParameter, statusReportClass,
            statusReportNameParameter, statusReportFederatesParameter, reliable, label, emptyTag);
         check(partial.size() == 2 && Integer.valueOf(3).equals(partial.get(subjectHandle))
               && Integer.valueOf(2).equals(partial.get(observerHandle)),
            "standard MOM synchronization status did not report partial achievement");
         check(requestStatus(subject, observer, recorder, callbackModel, encoder, statusRequestClass,
               statusRequestParameter, statusReportClass, statusReportNameParameter,
               statusReportFederatesParameter, reliable, "missing-java-tck-sync-point", emptyTag)
               .isEmpty(),
            "standard MOM synchronization status returned data for a missing label");

         recorder.clearInteraction();
         subject.sendInteraction(pointsRequestClass,
            subject.getParameterHandleValueMapFactory().create(0), emptyTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            observer.unsubscribeInteractionClass(pointsReportClass);
            pointsSubscribed = false;
            while (subject.evokeMultipleCallbacks(0.0, 0.0)
                  || observer.evokeMultipleCallbacks(0.0, 0.0)) {
            }
            check(recorder.interactions.isEmpty(),
               "queued MOM synchronization report arrived after evoked unsubscribe");
         } else {
            check(recorder.interactions.size() == 1,
               "immediate MOM synchronization report was not delivered inline");
            observer.unsubscribeInteractionClass(pointsReportClass);
            pointsSubscribed = false;
         }
         observer.subscribeInteractionClass(pointsReportClass);
         pointsSubscribed = true;

         observer.synchronizationPointAchieved(label);
         await(subject, observer, callbackModel,
            () -> subjectRecorder.synchronizedLabels.contains(label)
               && recorder.synchronizedLabels.contains(label),
            "standard synchronization completion callback");
         check(requestPointList(subject, observer, recorder, callbackModel, encoder,
               pointsRequestClass, pointsReportClass, pointsReportParameter, reliable, emptyTag)
               .isEmpty(),
            "standard MOM synchronization-point list retained a completed point");
      } finally {
         if (statusSubscribed && observerJoined && statusReportClass != null) {
            try { observer.unsubscribeInteractionClass(statusReportClass); } catch (Exception ignored) { }
         }
         if (pointsSubscribed && observerJoined && pointsReportClass != null) {
            try { observer.unsubscribeInteractionClass(pointsReportClass); } catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static List<String> requestPointList(RTIambassador subject, RTIambassador observer,
         Recorder recorder, CallbackModel model, EncoderFactory encoder,
         InteractionClassHandle requestClass, InteractionClassHandle reportClass,
         ParameterHandle reportParameter, TransportationTypeHandle reliable, byte[] tag)
         throws Exception {
      recorder.clearInteraction();
      subject.sendInteraction(requestClass,
         subject.getParameterHandleValueMapFactory().create(0), tag);
      InteractionReport report = awaitReport(subject, observer, recorder, model);
      verifyReport(report, reportClass, 1, reliable);
      check(report.parameters.containsKey(reportParameter),
         "standard MOM synchronization list report omitted HLAsyncPoints");
      HLAvariableArray<HLAunicodeString> points = encoder.createHLAvariableArray(
         index -> encoder.createHLAunicodeString());
      points.decode(report.parameters.get(reportParameter));
      java.util.ArrayList<String> result = new java.util.ArrayList<>();
      for (HLAunicodeString point : points) result.add(point.getValue());
      return result;
   }

   private static Map<FederateHandle, Integer> requestStatus(RTIambassador subject,
         RTIambassador observer, Recorder recorder, CallbackModel model, EncoderFactory encoder,
         InteractionClassHandle requestClass, ParameterHandle requestParameter,
         InteractionClassHandle reportClass, ParameterHandle reportNameParameter,
         ParameterHandle reportFederatesParameter, TransportationTypeHandle reliable,
         String label, byte[] tag) throws Exception {
      recorder.clearInteraction();
      ParameterHandleValueMap values = subject.getParameterHandleValueMapFactory().create(1);
      values.put(requestParameter, encoder.createHLAunicodeString(label).toByteArray());
      subject.sendInteraction(requestClass, values, tag);
      InteractionReport report = awaitReport(subject, observer, recorder, model);
      verifyReport(report, reportClass, 2, reliable);
      check(report.parameters.containsKey(reportNameParameter)
            && report.parameters.containsKey(reportFederatesParameter),
         "standard MOM synchronization status report omitted a parameter");
      HLAunicodeString returnedLabel = encoder.createHLAunicodeString();
      returnedLabel.decode(report.parameters.get(reportNameParameter));
      check(label.equals(returnedLabel.getValue()),
         "standard MOM synchronization status report returned a different label");

      HLAvariableArray<HLAfixedRecord> statuses = encoder.createHLAvariableArray(index -> {
         HLAfixedRecord record = encoder.createHLAfixedRecord();
         record.add(encoder.createHLAvariableArray(inner -> encoder.createHLAbyte()));
         record.add(encoder.createHLAinteger32BE());
         return record;
      });
      statuses.decode(report.parameters.get(reportFederatesParameter));
      Map<FederateHandle, Integer> result = new LinkedHashMap<>();
      for (HLAfixedRecord status : statuses) {
         @SuppressWarnings("unchecked")
         HLAvariableArray<HLAbyte> encodedHandle =
            (HLAvariableArray<HLAbyte>) status.get(0);
         byte[] handleBytes = encodedHandle.toByteArray();
         FederateHandle handle = observer.getFederateHandleFactory().decode(handleBytes, 0);
         int state = ((HLAinteger32BE) status.get(1)).getValue();
         check(handle != null && result.put(handle, state) == null,
            "standard MOM synchronization status contained a null or duplicate federate");
      }
      return result;
   }

   private static InteractionReport awaitReport(RTIambassador subject, RTIambassador observer,
         Recorder recorder, CallbackModel model) throws Exception {
      await(subject, observer, model, () -> recorder.interactions.size() == 1,
         "standard federation MOM interaction report");
      check(recorder.interactions.size() == 1,
         "standard federation MOM query returned multiple interaction reports");
      return recorder.interactions.get(0);
   }

   private static void verifyReport(InteractionReport report, InteractionClassHandle expected,
         int parameterCount, TransportationTypeHandle reliable) {
      check(expected.equals(report.interaction) && report.parameters.size() == parameterCount
            && report.tag.length == 0 && reliable.equals(report.transportation)
            && (report.regions == null || report.regions.isEmpty()) && report.producer == null,
         "standard federation MOM report had unexpected interaction metadata");
   }

   private static void await(RTIambassador first, RTIambassador second, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) throw new AssertionError("timed out waiting for " + description);
         if (model == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.01);
            second.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   private static void pump(RTIambassador first, RTIambassador second, CallbackModel model,
         int passes) throws Exception {
      for (int i = 0; i < passes; i++) {
         if (model == CallbackModel.HLA_EVOKED) {
            first.evokeMultipleCallbacks(0.0, 0.0);
            second.evokeMultipleCallbacks(0.0, 0.0);
         } else Thread.sleep(1L);
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   private static final class InteractionReport {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final RegionHandleSet regions;

      private InteractionReport(InteractionClassHandle interaction,
            Map<ParameterHandle, byte[]> parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            RegionHandleSet regions) {
         this.interaction = interaction;
         this.parameters = parameters;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regions = regions;
      }
   }

   private static final class Recorder {
      private final List<InteractionReport> interactions = new java.util.concurrent.CopyOnWriteArrayList<>();
      private final List<String> synchronizedLabels = new java.util.concurrent.CopyOnWriteArrayList<>();
      private final List<String> registrations = new java.util.concurrent.CopyOnWriteArrayList<>();
      private final Map<String, byte[]> announcements = new java.util.concurrent.ConcurrentHashMap<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               if ("receiveInteraction".equals(method.getName()) && args != null
                     && args.length == 6 && args[1] instanceof ParameterHandleValueMap) {
                  ParameterHandleValueMap source = (ParameterHandleValueMap) args[1];
                  Map<ParameterHandle, byte[]> copy = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) copy.put(handle, source.get(handle).clone());
                  interactions.add(new InteractionReport((InteractionClassHandle) args[0], copy,
                     args[2] instanceof byte[] ? ((byte[]) args[2]).clone() : new byte[0],
                     (TransportationTypeHandle) args[3], (FederateHandle) args[4],
                     (RegionHandleSet) args[5]));
               } else if ("synchronizationPointRegistrationSucceeded".equals(method.getName())) {
                  registrations.add((String) args[0]);
               } else if ("announceSynchronizationPoint".equals(method.getName())) {
                  announcements.put((String) args[0], ((byte[]) args[1]).clone());
               } else if ("federationSynchronized".equals(method.getName())) {
                  synchronizedLabels.add((String) args[0]);
               }
               return null;
            });
      }

      private void clearInteraction() { interactions.clear(); }
   }
}
