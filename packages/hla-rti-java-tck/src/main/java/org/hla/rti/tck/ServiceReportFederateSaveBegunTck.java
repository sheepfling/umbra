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
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

/** Verifies the standard MOM report for a successful Federate Save Begun call. */
final class ServiceReportFederateSaveBegunTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportFederateSaveBegunTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectEvents = new Recorder();
      Recorder peerEvents = new Recorder();
      Recorder observerEvents = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-federate-save-begun-" + suffix;
      String saveLabel = "java-tck-federate-save-begun-" + suffix;
      boolean subjectConnected = false, peerConnected = false, observerConnected = false;
      boolean created = false, subjectJoined = false, peerJoined = false, observerJoined = false;
      boolean reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(subjectEvents.proxy(), callbackModel);
         subjectConnected = true;
         peer.connect(peerEvents.proxy(), callbackModel);
         peerConnected = true;
         observer.connect(observerEvents.proxy(), callbackModel);
         observerConnected = true;

         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
            "java-tck-save-begun-subject-" + suffix, "java-tck", federation);
         subjectJoined = true;
         peer.joinFederationExecution("java-tck-save-begun-peer-" + suffix,
            "java-tck", federation);
         peerJoined = true;
         observer.joinFederationExecution("java-tck-save-begun-observer-" + suffix,
            "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS_NAME);
         InteractionClassHandle activeReportClass = reportClass;
         JavaTckSupport.check(activeReportClass != null
               && REPORT_CLASS_NAME.equals(observer.getInteractionClassName(activeReportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETER_NAMES) {
            ParameterHandle handle = observer.getParameterHandle(activeReportClass, name);
            JavaTckSupport.check(handle != null
                  && name.equals(observer.getParameterName(reportClass, handle)),
               "standard HLAreportServiceInvocation parameter lookup/name round-trip failed");
            reportParameters.put(name, handle);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup/name round-trip failed");

         for (RTIambassador ambassador : new RTIambassador[] {subject, peer, observer}) {
            ambassador.setServiceReportingSwitch(false);
            ambassador.setSendServiceReportsToFileSwitch(false);
         }
         observer.subscribeInteractionClass(activeReportClass);
         reportSubscribed = true;
         subject.setServiceReportingSwitch(true);

         subject.requestFederationSave(saveLabel);
         RTIambassador[] participants = {subject, peer, observer};
         await(callbackModel, participants,
            () -> subjectEvents.saveInitiations.equals(List.of(saveLabel))
               && peerEvents.saveInitiations.equals(List.of(saveLabel))
               && observerEvents.saveInitiations.equals(List.of(saveLabel)),
            "Federate Save Begun initiation callbacks");

         subject.federateSaveBegun();
         await(callbackModel, participants,
            () -> findReport(
               observerEvents.interactions, activeReportClass, reportParameters, encoder)
               != null,
            "FederateSaveBegun HLAreportServiceInvocation callback");
         Interaction report = findReport(
            observerEvents.interactions, activeReportClass, reportParameters, encoder);
         verifyReport(observer, encoder, subjectHandle, activeReportClass, reliable,
            reportParameters, report);

         peer.federateSaveBegun();
         observer.federateSaveBegun();
         subject.federateSaveComplete();
         peer.federateSaveComplete();
         observer.federateSaveComplete();
         await(callbackModel, participants,
            () -> subjectEvents.federationSaved.get() == 1
               && peerEvents.federationSaved.get() == 1
               && observerEvents.federationSaved.get() == 1,
            "Federate Save Begun save completion callbacks");
      } finally {
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (peerJoined) {
            try { peer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) { }
         }
         if (peerConnected) {
            try { peer.disconnect(); }
            catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static Interaction findReport(List<Interaction> interactions,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> parameters,
         EncoderFactory encoder) throws Exception {
      for (Interaction candidate : interactions) {
         if (!reportClass.equals(candidate.interaction)) continue;
         byte[] value = candidate.parameters.get(parameters.get("HLAservice"));
         if (value == null) continue;
         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(value);
         if ("FederateSaveBegun".equals(service.getValue())) return candidate;
      }
      return null;
   }

   private static void verifyReport(RTIambassador observer, EncoderFactory encoder,
         FederateHandle subject, InteractionClassHandle reportClass,
         TransportationTypeHandle reliable, Map<String, ParameterHandle> parameters,
         Interaction report) throws Exception {
      JavaTckSupport.check(report != null && reportClass.equals(report.interaction)
            && report.parameters.size() == REPORT_PARAMETER_NAMES.length
            && report.tag.length == 0 && report.transportation.equals(reliable)
            && report.regionsEmpty,
         "FederateSaveBegun report returned non-standard interaction metadata");
      for (String name : REPORT_PARAMETER_NAMES) {
         JavaTckSupport.check(report.parameters.containsKey(parameters.get(name)),
            "FederateSaveBegun report omitted a standard parameter");
      }
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         report.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(subject.equals(reportedFederate),
         "FederateSaveBegun report identified the wrong invoking federate");

      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(report.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(report.parameters.get(parameters.get("HLAsuccessIndicator")));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(parameters.get("HLAexception")));
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(report.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check("FederateSaveBegun".equals(service.getValue())
            && serviceType.getValue() == 0 && success.getValue()
            && exception.getValue().isEmpty() && serial.getValue() >= 0,
         "FederateSaveBegun report returned the wrong service result");

      HLAfixedRecord argumentPrototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), argumentPrototype);
      supplied.decode(report.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == 0,
         "FederateSaveBegun report included unexpected supplied arguments");
      HLAfixedRecord returned = argumentPrototype(encoder);
      returned.decode(report.parameters.get(parameters.get("HLAreturnedArgument")));
      JavaTckSupport.check(returned.size() == 3
            && ((HLAinteger32BE) returned.get(0)).getValue() == 34
            && ((HLAunicodeString) returned.get(1)).getValue().isEmpty()
            && "null".equals(((HLAunicodeString) returned.get(2)).getValue()),
         "FederateSaveBegun report returned the wrong Null argument representation");
   }

   private static HLAfixedRecord argumentPrototype(EncoderFactory encoder) {
      HLAfixedRecord record = encoder.createHLAfixedRecord();
      record.add(encoder.createHLAinteger32BE());
      record.add(encoder.createHLAunicodeString());
      record.add(encoder.createHLAunicodeString());
      return record;
   }

   private static void await(CallbackModel callbackModel, RTIambassador[] participants,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.isMet() && System.nanoTime() < deadline) {
         for (RTIambassador participant : participants) {
            if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(participant);
         }
         if (callbackModel != CallbackModel.HLA_EVOKED) Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   @FunctionalInterface
   private interface Condition { boolean isMet() throws Exception; }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<String> saveInitiations = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final AtomicInteger federationSaved = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("initiateFederateSave".equals(name)
                     && arguments != null && arguments.length == 1) {
                  saveInitiations.add((String) arguments[0]);
               } else if ("federationSaved".equals(name)) {
                  federationSaved.incrementAndGet();
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  event.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     event.parameters.put(handle, source.get(handle).clone());
                  }
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  event.regionsEmpty = regions == null || regions.isEmpty();
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK save-begun report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
