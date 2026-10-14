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
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies standard HLAreportMOMexception after malformed standard MIM parameters. */
final class FederateMomExceptionReportDeliveryTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception";
   private static final String SET_SWITCHES =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches";
   private static final String SYNCHRONIZATION_STATUS_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus";
   private static final String SYNCHRONIZATION_STATUS_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPointStatus";
   private static final String FOM_MODULE_DATA_REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData";
   private static final String FOM_MODULE_DATA_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData";
   private static final String SERVICE_INVOCATION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateMomExceptionReportDeliveryTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "MOM exception-report testing requires adapter-supplied logical-time configuration");
      RTIambassador rti = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-exception-report-" + suffix;
      String federateName = "java-tck-mom-exception-federate-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean subscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         rti.connect(recorder.proxy(), callbackModel);
         connected = true;
         rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         rti.joinFederationExecution(federateName, "java-tck", federation);
         joined = true;

         reportClass = rti.getInteractionClassHandle(REPORT_CLASS);
         ParameterHandle serviceParameter = rti.getParameterHandle(reportClass, "HLAservice");
         ParameterHandle exceptionParameter = rti.getParameterHandle(reportClass, "HLAexception");
         ParameterHandle parameterErrorParameter = rti.getParameterHandle(
            reportClass, "HLAparameterError");
         ParameterHandle federateParameter = rti.getParameterHandle(reportClass, "HLAfederate");
         TransportationTypeHandle reliable = rti.getTransportationTypeHandle("HLAreliable");
         FederateHandle joinedFederate = rti.getFederateHandle(federateName);
         JavaTckSupport.check(reportClass != null && serviceParameter != null
               && exceptionParameter != null && parameterErrorParameter != null
               && federateParameter != null && reliable != null && joinedFederate != null,
            "standard MIM HLAreportMOMexception lookup returned an invalid handle");
         byte[] expectedFederate = encode(joinedFederate);
         rti.subscribeInteractionClass(reportClass);
         subscribed = true;

         InteractionClassHandle setSwitches = rti.getInteractionClassHandle(SET_SWITCHES);
         ParameterHandle resignActionParameter = rti.getParameterHandle(
            setSwitches, "HLAautomaticResignAction");
         ParameterHandle serviceReportingParameter = rti.getParameterHandle(
            setSwitches, "HLAserviceReporting");
         JavaTckSupport.check(setSwitches != null && resignActionParameter != null
               && serviceReportingParameter != null,
            "standard HLAsetSwitches malformed-value lookup returned an invalid handle");
         ResignAction initialResignAction = rti.getAutomaticResignDirective();
         boolean initialServiceReporting = rti.getServiceReportingSwitch();

         sendMalformedValue(rti, setSwitches, resignActionParameter, 6, recorder,
            callbackModel, encoder, "out-of-domain HLAresignAction value");
         JavaTckSupport.check(recorder.events.size() == 1,
            "malformed HLAresignAction did not deliver exactly one HLAreportMOMexception");
         sendMalformedValue(rti, setSwitches, serviceReportingParameter, 2, recorder,
            callbackModel, encoder, "out-of-domain HLAswitch value");
         JavaTckSupport.check(recorder.events.size() == 2,
            "malformed HLAserviceReporting did not deliver exactly two cumulative reports");

         for (Report report : recorder.events) {
            verifyReport(report, reportClass, serviceParameter, exceptionParameter,
               parameterErrorParameter, federateParameter, reliable, expectedFederate,
               SET_SWITCHES, encoder);
         }
         JavaTckSupport.check(rti.getAutomaticResignDirective() == initialResignAction,
            "malformed HLAautomaticResignAction changed the joined federate's directive");
         JavaTckSupport.check(rti.getServiceReportingSwitch() == initialServiceReporting,
            "malformed HLAserviceReporting changed the joined federate's switch");
      } finally {
         if (subscribed && reportClass != null) {
            try { rti.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   static void runMissingParameter(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      runMissingParameter(factory, fom, mim, timeImplementation, callbackModel,
         SYNCHRONIZATION_STATUS_REQUEST, SYNCHRONIZATION_STATUS_REPORT,
         "synchronization-status request", "HLAsyncPointName");
   }

   static void runMissingFomModuleIndicator(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      runMissingParameter(factory, fom, mim, timeImplementation, callbackModel,
         FOM_MODULE_DATA_REQUEST, FOM_MODULE_DATA_REPORT,
         "FOM-module data request", "HLAFOMmoduleIndicator");
   }

   private static void runMissingParameter(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel, String requestInteraction,
         String relatedReportInteraction, String requestDescription, String missingParameter)
         throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "missing-parameter MOM exception reporting requires adapter-supplied logical-time configuration");
      RTIambassador rti = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-exception-missing-parameter-" + suffix;
      String federateName = "java-tck-mom-exception-missing-parameter-federate-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean reportSubscribed = false;
      boolean statusSubscribed = false;
      InteractionClassHandle reportClass = null;
      InteractionClassHandle statusReportClass = null;
      try {
         rti.connect(recorder.proxy(), callbackModel);
         connected = true;
         rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         rti.joinFederationExecution(federateName, "java-tck", federation);
         joined = true;

         reportClass = rti.getInteractionClassHandle(REPORT_CLASS);
         ParameterHandle serviceParameter = rti.getParameterHandle(reportClass, "HLAservice");
         ParameterHandle exceptionParameter = rti.getParameterHandle(reportClass, "HLAexception");
         ParameterHandle parameterErrorParameter = rti.getParameterHandle(
            reportClass, "HLAparameterError");
         ParameterHandle federateParameter = rti.getParameterHandle(reportClass, "HLAfederate");
         TransportationTypeHandle reliable = rti.getTransportationTypeHandle("HLAreliable");
         FederateHandle joinedFederate = rti.getFederateHandle(federateName);
         InteractionClassHandle requestClass = rti.getInteractionClassHandle(requestInteraction);
         statusReportClass = rti.getInteractionClassHandle(relatedReportInteraction);
         JavaTckSupport.check(reportClass != null && serviceParameter != null
               && exceptionParameter != null && parameterErrorParameter != null
               && federateParameter != null && reliable != null && joinedFederate != null
               && requestClass != null && statusReportClass != null,
            "standard MIM " + requestDescription
               + " exception-report lookup returned an invalid handle");
         byte[] expectedFederate = encode(joinedFederate);

         rti.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         rti.subscribeInteractionClass(statusReportClass);
         statusSubscribed = true;

         ParameterHandleValueMap noParameters =
            rti.getParameterHandleValueMapFactory().create(0);
         try {
            rti.sendInteraction(requestClass, noParameters, new byte[0]);
         } catch (Exception allowedSynchronousFailure) {
            // The standard HLAreportMOMexception callback is the portable result.
         }
         await(rti, callbackModel, () -> !recorder.events.isEmpty(),
            "HLAreportMOMexception for " + requestDescription
               + " missing " + missingParameter);
         settleCallbacks(rti, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 1,
            requestDescription + " missing " + missingParameter
               + " did not deliver exactly one MOM exception report");
         verifyReport(recorder.events.get(0), reportClass, serviceParameter, exceptionParameter,
            parameterErrorParameter, federateParameter, reliable, expectedFederate,
            requestInteraction, encoder);
      } finally {
         if (statusSubscribed && statusReportClass != null) {
            try { rti.unsubscribeInteractionClass(statusReportClass); }
            catch (Exception ignored) { }
         }
         if (reportSubscribed && reportClass != null) {
            try { rti.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   static void runServiceReportingPrecondition(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "service-reporting precondition testing requires adapter-supplied logical-time configuration");
      RTIambassador rti = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-exception-report-precondition-" + suffix;
      String federateName = "java-tck-mom-exception-report-precondition-federate-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean exceptionReportSubscribed = false;
      boolean serviceReportSubscribed = false;
      InteractionClassHandle exceptionReportClass = null;
      InteractionClassHandle serviceReportClass = null;
      try {
         rti.connect(recorder.proxy(), callbackModel);
         connected = true;
         rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         rti.joinFederationExecution(federateName, "java-tck", federation);
         joined = true;

         exceptionReportClass = rti.getInteractionClassHandle(REPORT_CLASS);
         ParameterHandle serviceParameter =
            rti.getParameterHandle(exceptionReportClass, "HLAservice");
         ParameterHandle exceptionParameter =
            rti.getParameterHandle(exceptionReportClass, "HLAexception");
         ParameterHandle parameterErrorParameter =
            rti.getParameterHandle(exceptionReportClass, "HLAparameterError");
         ParameterHandle federateParameter =
            rti.getParameterHandle(exceptionReportClass, "HLAfederate");
         TransportationTypeHandle reliable = rti.getTransportationTypeHandle("HLAreliable");
         FederateHandle joinedFederate = rti.getFederateHandle(federateName);
         serviceReportClass = rti.getInteractionClassHandle(SERVICE_INVOCATION_REPORT);
         InteractionClassHandle setSwitchesClass = rti.getInteractionClassHandle(SET_SWITCHES);
         ParameterHandle serviceReportingParameter =
            rti.getParameterHandle(setSwitchesClass, "HLAserviceReporting");
         JavaTckSupport.check(exceptionReportClass != null && serviceParameter != null
               && exceptionParameter != null && parameterErrorParameter != null
               && federateParameter != null && reliable != null && joinedFederate != null
               && serviceReportClass != null && setSwitchesClass != null
               && serviceReportingParameter != null,
            "standard MIM service-reporting precondition lookup returned an invalid handle");
         byte[] expectedFederate = encode(joinedFederate);

         rti.subscribeInteractionClass(exceptionReportClass);
         exceptionReportSubscribed = true;
         rti.subscribeInteractionClass(serviceReportClass);
         serviceReportSubscribed = true;
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "standard Service Reporting switch did not begin disabled");

         try {
            sendServiceReportingValue(rti, setSwitchesClass, serviceReportingParameter,
               encoder, 1);
         } catch (Exception allowedSynchronousFailure) {
            // The standard MOM report, not an exception class, is the portable oracle.
         }
         await(rti, callbackModel, () -> !recorder.events.isEmpty(),
            "HLAreportMOMexception for the Service Reporting subscription precondition");
         settleCallbacks(rti, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 1,
            "rejected Service Reporting enablement did not produce exactly one MOM exception report");
         verifyReport(recorder.events.get(0), exceptionReportClass, serviceParameter,
            exceptionParameter, parameterErrorParameter, federateParameter, reliable,
            expectedFederate,
            "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches",
            false, encoder);
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "rejected Service Reporting enablement changed the switch state");

         rti.unsubscribeInteractionClass(serviceReportClass);
         serviceReportSubscribed = false;
         sendServiceReportingValue(rti, setSwitchesClass, serviceReportingParameter, encoder, 1);
         JavaTckSupport.check(rti.getServiceReportingSwitch(),
            "Service Reporting did not enable after removing its report-subscription precondition");
         settleCallbacks(rti, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 1
               && recorder.events.get(0).interaction.equals(exceptionReportClass),
            "successful Service Reporting enablement delivered an unexpected subscribed report");

         sendServiceReportingValue(rti, setSwitchesClass, serviceReportingParameter, encoder, 0);
         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "Service Reporting did not return to its initial disabled value");
      } finally {
         if (serviceReportSubscribed && serviceReportClass != null) {
            try { rti.unsubscribeInteractionClass(serviceReportClass); }
            catch (Exception ignored) { }
         }
         if (exceptionReportSubscribed && exceptionReportClass != null) {
            try { rti.unsubscribeInteractionClass(exceptionReportClass); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void sendServiceReportingValue(RTIambassador rti,
         InteractionClassHandle setSwitchesClass, ParameterHandle serviceReportingParameter,
         EncoderFactory encoder, int value) throws Exception {
      HLAinteger32BE encoded = encoder.createHLAinteger32BE(value);
      ParameterHandleValueMap parameters = rti.getParameterHandleValueMapFactory().create(1);
      parameters.put(serviceReportingParameter, encoded.toByteArray());
      rti.sendInteraction(setSwitchesClass, parameters, new byte[0]);
   }

   private static void sendMalformedValue(RTIambassador rti,
         InteractionClassHandle setSwitches, ParameterHandle parameter, int value,
         Recorder recorder, CallbackModel callbackModel, EncoderFactory encoder,
         String description) throws Exception {
      HLAinteger32BE encoded = encoder.createHLAinteger32BE(value);
      ParameterHandleValueMap parameters = rti.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, encoded.toByteArray());
      int before = recorder.events.size();
      try {
         rti.sendInteraction(setSwitches, parameters, new byte[0]);
      } catch (Exception allowedSynchronousFailure) {
         // The standard MOM callback remains the portable result for malformed data.
      }
      await(rti, callbackModel, () -> recorder.events.size() > before,
         "HLAreportMOMexception for " + description);
      JavaTckSupport.check(recorder.events.size() == before + 1,
         description + " delivered an unexpected number of MOM exception reports");
   }

   private static void verifyReport(Report report, InteractionClassHandle reportClass,
         ParameterHandle serviceParameter, ParameterHandle exceptionParameter,
         ParameterHandle parameterErrorParameter, ParameterHandle federateParameter,
         TransportationTypeHandle reliable, byte[] expectedFederate, String expectedService,
         EncoderFactory encoder) throws Exception {
      verifyReport(report, reportClass, serviceParameter, exceptionParameter,
         parameterErrorParameter, federateParameter, reliable, expectedFederate,
         expectedService, true, encoder);
   }

   private static void verifyReport(Report report, InteractionClassHandle reportClass,
         ParameterHandle serviceParameter, ParameterHandle exceptionParameter,
         ParameterHandle parameterErrorParameter, ParameterHandle federateParameter,
         TransportationTypeHandle reliable, byte[] expectedFederate, String expectedService,
         boolean expectedParameterError, EncoderFactory encoder) throws Exception {
      JavaTckSupport.check(report.interaction.equals(reportClass)
            && report.parameters.size() == 4
            && report.parameters.containsKey(serviceParameter)
            && report.parameters.containsKey(exceptionParameter)
            && report.parameters.containsKey(parameterErrorParameter)
            && report.parameters.containsKey(federateParameter)
            && reliable.equals(report.transportation)
            && report.producer == null && report.regionsEmpty,
         "HLAreportMOMexception callback metadata or parameter set was not standard");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(serviceParameter));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(exceptionParameter));
      HLAboolean parameterError = encoder.createHLAboolean();
      parameterError.decode(report.parameters.get(parameterErrorParameter));
      JavaTckSupport.check(expectedService.equals(service.getValue()),
         "HLAreportMOMexception did not identify the expected fully qualified standard interaction");
      JavaTckSupport.check(!exception.getValue().isEmpty(),
         "HLAreportMOMexception did not provide a textual problem description");
      JavaTckSupport.check(parameterError.getValue() == expectedParameterError,
         expectedParameterError
            ? "HLAreportMOMexception did not mark the malformed parameter"
            : "HLAreportMOMexception incorrectly marked a service precondition failure as a parameter error");
      JavaTckSupport.check(Arrays.equals(expectedFederate,
            report.parameters.get(federateParameter)),
         "HLAreportMOMexception identified a different joined federate");
   }

   private static byte[] encode(FederateHandle federate) throws Exception {
      byte[] result = new byte[federate.encodedLength()];
      federate.encode(result, 0);
      return result;
   }

   private static void await(RTIambassador rti, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(rti);
         else Thread.yield();
         Thread.sleep(1L);
      }
   }

   private static void settleCallbacks(RTIambassador rti, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(rti);
      else Thread.sleep(25L);
   }

   private static FederateAmbassador callbackProxy() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK MOM exception callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (args == null ? null : args[0]);
            }
            return null;
         });
   }

   private static final class Report {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Report> events = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               if ("receiveInteraction".equals(method.getName()) && args != null
                     && args.length == 6 && args[1] instanceof ParameterHandleValueMap) {
                  Report report = new Report();
                  report.interaction = (InteractionClassHandle) args[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) args[1];
                  report.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     report.parameters.put(handle, source.get(handle).clone());
                  }
                  report.transportation = (TransportationTypeHandle) args[3];
                  report.producer = (FederateHandle) args[4];
                  report.regionsEmpty = args[5] == null || ((RegionHandleSet) args[5]).isEmpty();
                  events.add(report);
               }
               return callbackObjectMethod(proxy, method.getName(), args);
            });
      }
   }

   private static Object callbackObjectMethod(Object proxy, String name, Object[] args) {
      if ("toString".equals(name)) return "Java TCK MOM exception recorder";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
      return null;
   }
}
