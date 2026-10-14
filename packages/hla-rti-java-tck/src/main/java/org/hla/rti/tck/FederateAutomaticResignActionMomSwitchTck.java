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

/** Checks HLAsetSwitches mapping for the federate's automatic-resign directive. */
final class FederateAutomaticResignActionMomSwitchTck {
   private static final String SET_SWITCHES =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches";

   private FederateAutomaticResignActionMomSwitchTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador rti = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-auto-resign-mom-switch-" + suffix;
      String federate = "java-tck-auto-resign-switch-federate-" + suffix;
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
            "Service Reporting switch did not start false for Automatic Resign Action test");
         ResignAction initialAction = rti.getAutomaticResignDirective();
         boolean initialExceptionReporting = rti.getExceptionReportingSwitch();
         boolean initialFileReporting = rti.getSendServiceReportsToFileSwitch();

         InteractionClassHandle setSwitches = rti.getInteractionClassHandle(SET_SWITCHES);
         ParameterHandle automaticResignAction = rti.getParameterHandle(
            setSwitches, "HLAautomaticResignAction");
         JavaTckSupport.check(setSwitches != null && automaticResignAction != null,
            "standard federate-MOM Automatic Resign Action lookup returned an invalid handle");

         setResignAction(rti, setSwitches, automaticResignAction, ResignAction.NO_ACTION,
            initialExceptionReporting, initialFileReporting, encoder);
         setResignAction(rti, setSwitches, automaticResignAction, ResignAction.DELETE_OBJECTS,
            initialExceptionReporting, initialFileReporting, encoder);
         setResignAction(rti, setSwitches, automaticResignAction, initialAction,
            initialExceptionReporting, initialFileReporting, encoder);
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

   private static void setResignAction(RTIambassador rti, InteractionClassHandle interaction,
         ParameterHandle parameter, ResignAction action, boolean exceptionReporting,
         boolean fileReporting, EncoderFactory encoder) throws Exception {
      HLAinteger32BE value = encoder.createHLAinteger32BE(mimValue(action));
      ParameterHandleValueMap parameters = rti.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value.toByteArray());
      rti.sendInteraction(interaction, parameters, new byte[0]);
      JavaTckSupport.check(rti.getAutomaticResignDirective() == action,
         "HLAsetSwitches HLAautomaticResignAction did not set " + action);
      JavaTckSupport.check(!rti.getServiceReportingSwitch()
            && rti.getExceptionReportingSwitch() == exceptionReporting
            && rti.getSendServiceReportsToFileSwitch() == fileReporting,
         "HLAsetSwitches HLAautomaticResignAction changed a neighboring federate switch");
   }

   private static int mimValue(ResignAction action) {
      if (action == ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES) return 0;
      if (action == ResignAction.DELETE_OBJECTS) return 1;
      if (action == ResignAction.CANCEL_PENDING_OWNERSHIP_ACQUISITIONS) return 2;
      if (action == ResignAction.DELETE_OBJECTS_THEN_DIVEST) return 3;
      if (action == ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST) return 4;
      if (action == ResignAction.NO_ACTION) return 5;
      throw new AssertionError("unmapped standard ResignAction: " + action);
   }

   private static FederateAmbassador callbackProxy() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK automatic-resign callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (args == null ? null : args[0]);
            }
            return null;
         });
   }
}
