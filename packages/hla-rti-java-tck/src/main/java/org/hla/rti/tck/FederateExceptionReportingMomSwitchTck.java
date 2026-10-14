package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Verifies the standard federate-MOM HLAexceptionReporting switch in isolation. */
final class FederateExceptionReportingMomSwitchTck {
   private static final String SET_SWITCHES =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches";
   private FederateExceptionReportingMomSwitchTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-exception-reporting-mom-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false;
      try {
         ambassador.connect(callbackProxy(), callbackModel);
         connected = true;
         ambassador.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         ambassador.joinFederationExecution("exception-reporting-mom-federate", federation);
         joined = true;

         InteractionClassHandle setSwitches = ambassador.getInteractionClassHandle(SET_SWITCHES);
         ParameterHandle exceptionReporting = ambassador.getParameterHandle(setSwitches,
            "HLAexceptionReporting");
         JavaTckSupport.check(setSwitches != null && exceptionReporting != null,
            "standard federate-MOM Exception Reporting lookup returned an invalid handle");
         JavaTckSupport.check(!ambassador.getExceptionReportingSwitch(),
            "standard MIM Exception Reporting switch did not start false");

         boolean initialServiceReporting = ambassador.getServiceReportingSwitch();
         ResignAction initialResignAction = ambassador.getAutomaticResignDirective();
         boolean initialFileReporting = ambassador.getSendServiceReportsToFileSwitch();
         setSwitch(ambassador, setSwitches, exceptionReporting, true, encoder,
            initialServiceReporting, initialResignAction, initialFileReporting);
         setSwitch(ambassador, setSwitches, exceptionReporting, false, encoder,
            initialServiceReporting, initialResignAction, initialFileReporting);

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         ambassador.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { ambassador.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void setSwitch(RTIambassador ambassador, InteractionClassHandle interaction,
         ParameterHandle parameter, boolean enabled, EncoderFactory encoder,
         boolean serviceReporting, ResignAction resignAction, boolean fileReporting)
         throws Exception {
      HLAinteger32BE value = encoder.createHLAinteger32BE(enabled ? 1 : 0);
      ParameterHandleValueMap parameters = ambassador.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value.toByteArray());
      ambassador.sendInteraction(interaction, parameters, new byte[0]);
      JavaTckSupport.check(ambassador.getExceptionReportingSwitch() == enabled,
         "HLAsetSwitches HLAexceptionReporting did not set the switch to " + enabled);
      JavaTckSupport.check(ambassador.getServiceReportingSwitch() == serviceReporting
            && ambassador.getAutomaticResignDirective() == resignAction
            && ambassador.getSendServiceReportsToFileSwitch() == fileReporting,
         "HLAsetSwitches HLAexceptionReporting changed a neighboring federate switch");
   }

   private static FederateAmbassador callbackProxy() {
      return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK federate MOM callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (args == null ? null : args[0]);
            }
            return null;
         });
   }
}
