package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks the standard HLAsetSwitches path for the file-reporting switch. */
final class FederateSendServiceReportsToFileMomSwitchTck {
   private static final String SET_SWITCHES =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches";

   private FederateSendServiceReportsToFileMomSwitchTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador rti = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-send-service-reports-file-" + suffix;
      String federate = "java-tck-report-file-switch-federate-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         rti.connect(callbackProxy(), callbackModel);
         connected = true;
         rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         rti.joinFederationExecution(federate, "java-tck", federation);
         joined = true;

         JavaTckSupport.check(!rti.getServiceReportingSwitch(),
            "Service Reporting switch did not start false; file reporting must remain inactive");
         boolean initialFileReporting = rti.getSendServiceReportsToFileSwitch();
         boolean initialExceptionReporting = rti.getExceptionReportingSwitch();
         ResignAction initialResignAction = rti.getAutomaticResignDirective();

         InteractionClassHandle setSwitches = rti.getInteractionClassHandle(SET_SWITCHES);
         ParameterHandle sendReportsToFile = rti.getParameterHandle(
            setSwitches, "HLAsendServiceReportsToFile");
         JavaTckSupport.check(setSwitches != null && sendReportsToFile != null,
            "standard federate-MOM report-destination lookup returned an invalid handle");

         setFileSwitch(rti, setSwitches, sendReportsToFile, false, encoder,
            initialExceptionReporting, initialResignAction);
         setFileSwitch(rti, setSwitches, sendReportsToFile, true, encoder,
            initialExceptionReporting, initialResignAction);
         setFileSwitch(rti, setSwitches, sendReportsToFile, initialFileReporting, encoder,
            initialExceptionReporting, initialResignAction);
      } finally {
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

   private static void setFileSwitch(RTIambassador rti, InteractionClassHandle interaction,
         ParameterHandle parameter, boolean enabled, EncoderFactory encoder,
         boolean exceptionReporting, ResignAction resignAction) throws Exception {
      HLAinteger32BE value = encoder.createHLAinteger32BE(enabled ? 1 : 0);
      ParameterHandleValueMap parameters = rti.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value.toByteArray());
      rti.sendInteraction(interaction, parameters, new byte[0]);
      JavaTckSupport.check(rti.getSendServiceReportsToFileSwitch() == enabled,
         "HLAsetSwitches HLAsendServiceReportsToFile did not set the switch to " + enabled);
      JavaTckSupport.check(!rti.getServiceReportingSwitch()
            && rti.getExceptionReportingSwitch() == exceptionReporting
            && rti.getAutomaticResignDirective() == resignAction,
         "HLAsetSwitches HLAsendServiceReportsToFile changed a neighboring federate switch");
   }

   private static FederateAmbassador callbackProxy() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK report-file callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (args == null ? null : args[0]);
            }
            return null;
         });
   }
}
