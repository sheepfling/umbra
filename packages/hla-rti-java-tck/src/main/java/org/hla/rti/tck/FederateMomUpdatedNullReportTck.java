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
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAoctet;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies the standard empty updated-object MOM response for a federate with no updates. */
final class FederateMomUpdatedNullReportTck {
   private static final String REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated";
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesUpdated";
   private static final String REFLECTED_REQUEST_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected";
   private static final String REFLECTED_REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesReflected";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomUpdatedNullReportTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, callbackModel, false);
   }

   static void runReflected(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, mim, timeImplementation, callbackModel, true);
   }

   private static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel, boolean reflectedReport) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-federate-mom-updated-null-report-" + UUID.randomUUID();
      boolean subjectConnected = false, requesterConnected = false, created = false;
      boolean subjectJoined = false, requesterJoined = false, subscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(new Recorder().proxy(), callbackModel);
         subjectConnected = true;
         requester.connect(recorder.proxy(), callbackModel);
         requesterConnected = true;
         subject.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "mom-updated-null-subject", "java-tck", federation);
         subjectJoined = true;
         requester.joinFederationExecution("mom-updated-null-requester", "java-tck",
            federation);
         requesterJoined = true;

         String requestClassName = reflectedReport ? REFLECTED_REQUEST_CLASS : REQUEST_CLASS;
         String reportClassName = reflectedReport ? REFLECTED_REPORT_CLASS : REPORT_CLASS;
         InteractionClassHandle requestClass = requester.getInteractionClassHandle(requestClassName);
         ParameterHandle requestFederate = requester.getParameterHandle(requestClass, "HLAfederate");
         reportClass = requester.getInteractionClassHandle(reportClassName);
         ParameterHandle reportCounts = requester.getParameterHandle(reportClass,
            "HLAobjectInstanceCounts");
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(requestClassName.equals(requester.getInteractionClassName(requestClass))
               && "HLAfederate".equals(requester.getParameterName(requestClass, requestFederate))
               && reportClassName.equals(requester.getInteractionClassName(reportClass))
               && "HLAobjectInstanceCounts".equals(requester.getParameterName(reportClass,
                  reportCounts))
               && "HLAreliable".equals(requester.getTransportationTypeName(reliable)),
            "standard updated-object request/report handles did not round-trip");
         requester.subscribeInteractionClass(reportClass);
         subscribed = true;

         int reportStart = recorder.interactions.size();
         ParameterHandleValueMap requestParameters =
            requester.getParameterHandleValueMapFactory().create(1);
         requestParameters.put(requestFederate, encode(subjectFederate));
         requester.sendInteraction(requestClass, requestParameters, new byte[0]);
         InteractionClassHandle resolvedReportClass = reportClass;
         await(requester, callbackModel,
            () -> findReport(recorder, reportStart, resolvedReportClass, reportCounts) != null,
            "standard NULL MOM object-count response");
         List<Interaction> reports = reportsSince(recorder, reportStart, resolvedReportClass);
         JavaTckSupport.check(reports.size() == 1,
            "joined subject with no qualifying callbacks did not produce exactly one report");
         Interaction report = reports.get(0);
         JavaTckSupport.check(report.parameters.size() == 1
               && report.parameters.containsKey(reportCounts)
               && reliable.equals(report.transportation) && report.tag.length == 0
               && report.producer == null && report.regionsEmpty,
            "empty object-count report did not preserve its reliable one-parameter response metadata");

         EncoderFactory encoder = factory.getEncoderFactory();
         HLAvariableArray<HLAfixedRecord> decodedCounts = emptyCountArray(encoder);
         byte[] encodedCounts = report.parameters.get(reportCounts);
         if (encodedCounts.length != 0) decodedCounts.decode(encodedCounts);
         JavaTckSupport.check(decodedCounts.size() == 0,
            "standard NULL object-count report encoded a non-empty class-count array");

         requester.unsubscribeInteractionClass(reportClass);
         subscribed = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (subscribed && requesterJoined && reportClass != null) try {
            requester.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (requesterJoined) resign(requester);
         if (subjectJoined) resign(subject);
         if (created) try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) disconnect(requester);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static HLAvariableArray<HLAfixedRecord> emptyCountArray(EncoderFactory encoder)
         throws Exception {
      HLAvariableArray<HLAoctet> handlePrototype = encoder.createHLAvariableArray(
         index -> encoder.createHLAoctet(), encoder.createHLAoctet());
      HLAfixedRecord recordPrototype = encoder.createHLAfixedRecord();
      recordPrototype.add(handlePrototype);
      recordPrototype.add(encoder.createHLAinteger32BE());
      return encoder.createHLAvariableArray(index -> {
         HLAfixedRecord record = encoder.createHLAfixedRecord();
         record.add(encoder.createHLAvariableArray(
            inner -> encoder.createHLAoctet(), encoder.createHLAoctet()));
         record.add(encoder.createHLAinteger32BE());
         return record;
      }, recordPrototype);
   }

   private static Interaction findReport(Recorder recorder, int first,
         InteractionClassHandle reportClass, ParameterHandle counts) {
      for (int i = Math.min(first, recorder.interactions.size()); i < recorder.interactions.size(); ++i) {
         Interaction interaction = recorder.interactions.get(i);
         if (reportClass.equals(interaction.interaction) && interaction.parameters.containsKey(counts))
            return interaction;
      }
      return null;
   }

   private static List<Interaction> reportsSince(Recorder recorder, int first,
         InteractionClassHandle reportClass) {
      List<Interaction> result = new ArrayList<>();
      for (int i = Math.min(first, recorder.interactions.size()); i < recorder.interactions.size(); ++i) {
         Interaction interaction = recorder.interactions.get(i);
         if (reportClass.equals(interaction.interaction)) result.add(interaction);
      }
      return result;
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
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
   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     (ParameterHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (RegionHandleSet) arguments[5]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK updated-null MOM report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
