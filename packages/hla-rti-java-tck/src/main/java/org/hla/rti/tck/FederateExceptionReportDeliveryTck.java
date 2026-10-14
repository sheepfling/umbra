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

/** Verifies standard HLAreportException delivery follows the Exception Reporting switch. */
final class FederateExceptionReportDeliveryTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException";
   private static final String SET_SWITCHES =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private FederateExceptionReportDeliveryTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "Exception-report delivery requires adapter-supplied logical-time configuration");
      RTIambassador rti = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder recorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-exception-report-delivery-" + suffix;
      String federateName = "java-tck-exception-report-federate-" + suffix;
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
         ParameterHandle federateParameter = rti.getParameterHandle(reportClass, "HLAfederate");
         TransportationTypeHandle reliable = rti.getTransportationTypeHandle("HLAreliable");
         FederateHandle joinedFederate = rti.getFederateHandle(federateName);
         JavaTckSupport.check(reportClass != null && serviceParameter != null
               && exceptionParameter != null && federateParameter != null
               && reliable != null && joinedFederate != null,
            "standard MIM HLAreportException lookup returned an invalid handle");
         byte[] expectedFederate = encode(joinedFederate);
         rti.subscribeInteractionClass(reportClass);
         subscribed = true;

         JavaTckSupport.check(!rti.getExceptionReportingSwitch()
               && !rti.getServiceReportingSwitch(),
            "standard MIM Exception Reporting or Service Reporting switch did not start false");
         String disabledException = failObjectClassLookup(rti);
         JavaTckSupport.check(!disabledException.isEmpty(),
            "undefined object-class lookup did not preserve NameNotFound details");
         settleCallbacks(rti, callbackModel);
         JavaTckSupport.check(recorder.events.isEmpty(),
            "HLAreportException arrived while Exception Reporting was disabled");

         InteractionClassHandle setSwitches = rti.getInteractionClassHandle(SET_SWITCHES);
         ParameterHandle exceptionReporting = rti.getParameterHandle(
            setSwitches, "HLAexceptionReporting");
         JavaTckSupport.check(setSwitches != null && exceptionReporting != null,
            "standard HLAsetSwitches Exception Reporting lookup returned an invalid handle");
         setExceptionReporting(rti, setSwitches, exceptionReporting, true, encoder);

         String enabledException = failObjectClassLookup(rti);
         JavaTckSupport.check(!enabledException.isEmpty(),
            "undefined object-class lookup did not preserve NameNotFound details");
         await(rti, callbackModel, () -> recorder.events.size() >= 1,
            "HLAreportException while Exception Reporting is enabled");
         JavaTckSupport.check(recorder.events.size() == 1,
            "enabled Exception Reporting did not deliver exactly one HLAreportException");
         verifyReport(recorder.events.get(0), reportClass, serviceParameter, exceptionParameter,
            federateParameter, reliable, expectedFederate, encoder);

         setExceptionReporting(rti, setSwitches, exceptionReporting, false, encoder);
         String disabledAgainException = failObjectClassLookup(rti);
         JavaTckSupport.check(!disabledAgainException.isEmpty(),
            "second undefined object-class lookup did not preserve NameNotFound details");
         settleCallbacks(rti, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 1,
            "HLAreportException arrived after Exception Reporting was disabled");
         verifyReport(recorder.events.get(0), reportClass, serviceParameter, exceptionParameter,
            federateParameter, reliable, expectedFederate, encoder);
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

   private static String failObjectClassLookup(RTIambassador rti) throws Exception {
      try {
         String unknownObjectClassName = "HLAobjectRoot.UndefinedReportProbe"
            + UUID.randomUUID().toString().replace("-", "");
         rti.getObjectClassHandle(unknownObjectClassName);
      } catch (Throwable error) {
         for (Throwable current = error; current != null; current = current.getCause()) {
            if ("NameNotFound".equals(current.getClass().getSimpleName())) {
               return error.toString();
            }
         }
         throw new AssertionError("undefined object-class lookup threw the wrong exception", error);
      }
      throw new AssertionError("undefined object-class lookup completed without NameNotFound");
   }

   private static void setExceptionReporting(RTIambassador rti, InteractionClassHandle interaction,
         ParameterHandle parameter, boolean enabled, EncoderFactory encoder) throws Exception {
      HLAinteger32BE value = encoder.createHLAinteger32BE(enabled ? 1 : 0);
      ParameterHandleValueMap parameters = rti.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value.toByteArray());
      rti.sendInteraction(interaction, parameters, new byte[0]);
      JavaTckSupport.check(rti.getExceptionReportingSwitch() == enabled,
         "HLAsetSwitches did not update the Exception Reporting switch to " + enabled);
   }

   private static void verifyReport(Report report, InteractionClassHandle reportClass,
         ParameterHandle serviceParameter, ParameterHandle exceptionParameter,
         ParameterHandle federateParameter, TransportationTypeHandle reliable,
         byte[] expectedFederate, EncoderFactory encoder) throws Exception {
      JavaTckSupport.check(report.interaction.equals(reportClass)
            && report.parameters.size() == 3
            && report.parameters.containsKey(serviceParameter)
            && report.parameters.containsKey(exceptionParameter)
            && report.parameters.containsKey(federateParameter)
            && reliable.equals(report.transportation)
            && report.producer == null && report.regionsEmpty,
         "HLAreportException callback metadata or parameter set was not standard");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(report.parameters.get(serviceParameter));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(report.parameters.get(exceptionParameter));
      JavaTckSupport.check(!service.getValue().isEmpty() && !exception.getValue().isEmpty(),
         "HLAreportException service or exception text was empty");
      JavaTckSupport.check(Arrays.equals(expectedFederate,
            report.parameters.get(federateParameter)),
         "HLAreportException identified a different joined federate");
   }

   private static byte[] encode(FederateHandle federate) throws Exception {
      byte[] result = new byte[federate.encodedLength()];
      federate.encode(result, 0);
      return result;
   }

   private static void settleCallbacks(RTIambassador rti, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(rti);
      else Thread.sleep(25L);
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

   private static FederateAmbassador callbackProxy() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK exception report callbacks";
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
                  ParameterHandleValueMap values = (ParameterHandleValueMap) args[1];
                  report.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : values.keySet()) {
                     report.parameters.put(handle, values.get(handle).clone());
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
      if ("toString".equals(name)) return "Java TCK exception report recorder";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
      return null;
   }
}
